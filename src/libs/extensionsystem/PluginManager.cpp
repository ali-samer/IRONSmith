#include "PluginManager.hpp"

#include <QtCore/QDebug>
#include <QtCore/QSet>
#include <QtCore/QPluginLoader>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>
#include <QtCore/QFileInfo>
#include <QtCore/QLibrary>

namespace ExtensionSystem {

namespace {

QString fsPathToQString(const std::filesystem::path& p)
{
#if defined(__cpp_char8_t)
	const std::u8string u8 = p.u8string();
	return QString::fromUtf8(reinterpret_cast<const char*>(u8.data()), static_cast<int>(u8.size()));
#else
	const std::string u8 = p.u8string();
	return QString::fromUtf8(u8.data(), static_cast<int>(u8.size()));
#endif
}

QStringList parseDependencies(const QJsonValue& depsVal)
{
	QStringList deps;
	if (!depsVal.isArray())
		return deps;

	const QJsonArray arr = depsVal.toArray();
	for (const QJsonValue& v : arr) {
		if (v.isString()) {
			const QString s = v.toString().trimmed();
			if (!s.isEmpty())
				deps.push_back(s);
			continue;
		}
		if (v.isObject()) {
			const QJsonObject o = v.toObject();
			const QString name = o.value(QStringLiteral("Name")).toString().trimmed();
			if (!name.isEmpty())
				deps.push_back(name);
		}
	}

	deps.removeDuplicates();
	deps.sort(Qt::CaseSensitive);
	return deps;
}

} // namespace

PluginManager& PluginManager::instance()
{
    static PluginManager pm;
    return pm;
}

PluginManager::PluginManager(QObject* parent)
    : QObject(parent)
{
}

PluginManager::~PluginManager()
{
	for (IPlugin* p : m_plugins) {
		if (!m_loaderOwnedPlugins.contains(p))
			delete p;
	}
    m_plugins.clear();

	for (auto it = m_loadersById.begin(); it != m_loadersById.end(); ++it) {
		if (QPluginLoader* loader = it.value()) {
			loader->unload();
			delete loader;
		}
	}
	m_loadersById.clear();
	m_loaderOwnedPlugins.clear();
    m_objects.clear();
}

void PluginManager::setPluginPaths(const std::vector<std::filesystem::path>& paths)
{
    auto& self = instance();
    self.m_pluginPaths.clear();
    self.m_pluginPaths.reserve(static_cast<int>(paths.size()));

#if defined(__cpp_char8_t)
    for (const auto& p : paths) {
        const std::u8string u8 = p.u8string();
        self.m_pluginPaths.push_back(
            QString::fromUtf8(reinterpret_cast<const char*>(u8.data()),
                              static_cast<int>(u8.size()))
        );
    }
#else
    for (const auto& p : paths) {
        const std::string u8 = p.u8string();
        self.m_pluginPaths.push_back(QString::fromUtf8(u8.data(), static_cast<int>(u8.size())));
    }
#endif
}

PluginSpec* PluginManager::specById(const char* id)
{
    return specById(QString::fromUtf8(id));
}

PluginSpec* PluginManager::specById(const QString& id)
{
    auto& self = instance();
    auto it = self.m_specs.find(id);
    if (it == self.m_specs.end())
        return nullptr;
    return &it.value();
}

QStringList PluginManager::lastErrors()
{
    return instance().m_lastErrors;
}

bool PluginManager::isValidId(const QString& id)
{
    if (id.isEmpty())
        return false;

    for (QChar c : id) {
        if (!(c.isLetterOrNumber() || c == '_' || c == '-' || c == '.'))
            return false;
    }
    return true;
}

void PluginManager::registerPlugin(PluginSpec spec)
{
    auto& self = instance();
    const QString id = spec.id();

    if (!isValidId(id)) {
        self.m_lastErrors.push_back(QString("Invalid plugin id '%1'.").arg(id));
        return;
    }

    if (self.m_specs.contains(id)) {
        PluginSpec& existing = self.m_specs[id];
        existing.addError(QString("Duplicate plugin id '%1' registered.").arg(id));

        self.m_lastErrors.push_back(QString("Duplicate plugin id '%1' registered.").arg(id));
        return;
    }

    self.m_specs.insert(id, std::move(spec));
}

	void PluginManager::clearRegistrationState()
	{
		m_lastErrors.clear();
		m_specs.clear();
		m_loadOrder.clear();

		for (auto it = m_loadersById.begin(); it != m_loadersById.end(); ++it) {
			if (::QPluginLoader* loader = it.value()) {
				loader->unload();
				delete loader;
			}
		}
		m_loadersById.clear();
		m_loaderOwnedPlugins.clear();
		m_plugins.clear();
		m_objects.clear();
	}

bool PluginManager::registerPlugins(const QStringList& pluginFiles)
{
	auto& self = instance();
		self.clearRegistrationState();

	bool ok = true;
	for (const QString& file : pluginFiles) {
		const QFileInfo fi(file);
		if (!fi.exists() || !fi.isFile()) {
			self.m_lastErrors.push_back(QString("Plugin file does not exist: %1").arg(file));
			ok = false;
			continue;
		}
		if (!QLibrary::isLibrary(fi.absoluteFilePath())) {
			self.m_lastErrors.push_back(QString("Not a loadable library: %1").arg(fi.absoluteFilePath()));
			ok = false;
			continue;
		}

		auto* loader = new QPluginLoader(fi.absoluteFilePath());
		const QJsonObject root = loader->metaData();
		if (root.isEmpty()) {
			self.m_lastErrors.push_back(QString("Failed to read plugin metadata from %1: %2")
								.arg(fi.absoluteFilePath(), loader->errorString()));
			delete loader;
			ok = false;
			continue;
		}

		const QString iid = root.value(QStringLiteral("IID")).toString();
		if (iid != QStringLiteral("org.ironsmith.plugin")) {
			self.m_lastErrors.push_back(QString("Plugin %1 has unexpected IID '%2'.")
								.arg(fi.absoluteFilePath(), iid));
			delete loader;
			ok = false;
			continue;
		}

		const QJsonObject meta = root.value(QStringLiteral("MetaData")).toObject();
		if (meta.isEmpty()) {
			self.m_lastErrors.push_back(QString("Plugin %1 has no MetaData object.").arg(fi.absoluteFilePath()));
			delete loader;
			ok = false;
			continue;
		}

		const QString id = meta.value(QStringLiteral("Name")).toString().trimmed();
		if (!isValidId(id)) {
			self.m_lastErrors.push_back(QString("Plugin %1 has invalid Name '%2'.").arg(fi.absoluteFilePath(), id));
			delete loader;
			ok = false;
			continue;
		}

		if (self.m_specs.contains(id)) {
			self.m_lastErrors.push_back(QString("Duplicate plugin id '%1' from %2.").arg(id, fi.absoluteFilePath()));
			delete loader;
			ok = false;
			continue;
		}

		const QStringList deps = parseDependencies(meta.value(QStringLiteral("Dependencies")));

		self.m_loadersById.insert(id, loader);

		PluginSpec spec(id, deps,
			[loader, id]() -> IPlugin* {
				QObject* obj = loader->instance();
				if (!obj)
					return nullptr;
				return qobject_cast<IPlugin*>(obj);
			}
		);

		registerPlugin(std::move(spec));
	}

	return ok && self.m_lastErrors.isEmpty();
}

bool PluginManager::registerPlugins(const std::vector<std::filesystem::path>& pluginFiles)
{
	QStringList files;
	files.reserve(static_cast<int>(pluginFiles.size()));
	for (const auto& p : pluginFiles)
		files.push_back(fsPathToQString(p));
	return registerPlugins(files);
}

bool PluginManager::validateGraph(QStringList& errors) const
{
    for (auto it = m_specs.begin(); it != m_specs.end(); ++it) {
        const QString& id = it.key();
        const PluginSpec& spec = it.value();
        if (spec.hasError()) {
            for (const QString& e : spec.errors())
                errors.push_back(QString("Plugin '%1': %2").arg(id, e));
        }
    }
    if (!errors.isEmpty())
        return false;

    QSet<QString> ids;
    ids.reserve(m_specs.size());
    for (auto it = m_specs.begin(); it != m_specs.end(); ++it)
        ids.insert(it.key());

    for (auto it = m_specs.begin(); it != m_specs.end(); ++it) {
        const QString& id = it.key();
        const PluginSpec& spec = it.value();

        for (const QString& dep : spec.dependencies()) {
            if (dep == id) {
                errors.push_back(QString("Plugin '%1' depends on itself.").arg(id));
                continue;
            }
            if (!ids.contains(dep)) {
                errors.push_back(QString("Plugin '%1' depends on missing plugin '%2'.").arg(id, dep));
            }
        }
    }

    return errors.isEmpty();
}

bool PluginManager::findCycle(QStringList& cycleOut) const
{
    QHash<QString, QStringList> deps;
    deps.reserve(m_specs.size());
    for (auto it = m_specs.begin(); it != m_specs.end(); ++it)
        deps.insert(it.key(), it.value().dependencies());

    QHash<QString, int> color;
    color.reserve(m_specs.size());

    QHash<QString, QString> parent;
    parent.reserve(m_specs.size());

    auto buildCycle = [&](const QString& dep, const QString& u) -> bool {
        QStringList rev;
        QString x = u;
        rev.push_back(x);
        while (x != dep) {
            x = parent.value(x);
            if (x.isEmpty())
                return false;
            rev.push_back(x);
        }

        QStringList cycle;
        for (int i = rev.size() - 1; i >= 0; --i)
            cycle.push_back(rev[i]);
        cycle.push_back(dep);

        cycleOut = std::move(cycle);
        return true;
    };

    std::function<bool(const QString&)> dfs = [&](const QString& u) -> bool {
        color[u] = 1;
        const auto& ds = deps.value(u);

        for (const QString& v : ds) {
            const int cv = color.value(v, 0);
            if (cv == 0) {
                parent[v] = u;
                if (dfs(v))
                    return true;
            } else if (cv == 1) {
                if (buildCycle(v, u))
                    return true;
                cycleOut = {v, u, v};
                return true;
            }
        }

        color[u] = 2;
        return false;
    };

    QStringList ids = m_specs.keys();
    ids.sort(Qt::CaseSensitive);

    for (const QString& id : ids) {
        if (color.value(id, 0) == 0) {
            parent.remove(id);
            if (dfs(id))
                return true;
        }
    }

    return false;
}

bool PluginManager::computeLoadOrder(QVector<QString>& order, QStringList& errors) const
{
    if (!validateGraph(errors))
        return false;

    QHash<QString, int> indegree;
    QHash<QString, QVector<QString>> adj;

    indegree.reserve(m_specs.size());
    adj.reserve(m_specs.size());

    QStringList ids = m_specs.keys();
    ids.sort(Qt::CaseSensitive);

    for (const QString& id : ids) {
        indegree.insert(id, 0);
        adj.insert(id, {});
    }

    for (auto it = m_specs.begin(); it != m_specs.end(); ++it) {
        const QString& id = it.key();
        const QStringList deps = it.value().dependencies();
        for (const QString& dep : deps) {
            adj[dep].push_back(id);
            indegree[id] = indegree[id] + 1;
        }
    }

    QStringList queue;
    for (const QString& id : ids) {
        if (indegree.value(id) == 0)
            queue.push_back(id);
    }
    queue.sort(Qt::CaseSensitive);

    order.clear();
    order.reserve(m_specs.size());

    while (!queue.isEmpty()) {
        const QString u = queue.front();
        queue.pop_front();
        order.push_back(u);

        auto dependents = adj.value(u);
        std::sort(dependents.begin(), dependents.end(), [](const QString& a, const QString& b) {
            return a < b;
        });

        for (const QString& v : dependents) {
            indegree[v] = indegree[v] - 1;
            if (indegree[v] == 0) {
                queue.push_back(v);
                queue.sort(Qt::CaseSensitive);
            }
        }
    }

    if (order.size() != m_specs.size()) {
        QStringList cycle;
        if (findCycle(cycle) && cycle.size() >= 2) {
            errors.push_back(QString("Dependency cycle detected: %1").arg(cycle.join(" -> ")));
        } else {
            errors.push_back("Dependency cycle detected in plugin graph.");
        }
        return false;
    }

    return true;
}

bool PluginManager::loadPlugins(const QStringList& arguments)
{
    auto& self = instance();
    self.m_lastErrors.clear();
    self.m_plugins.clear();
	self.m_loaderOwnedPlugins.clear();

    QVector<QString> order;
    QStringList errors;
    if (!self.computeLoadOrder(order, errors)) {
        self.m_lastErrors = std::move(errors);
        return false;
    }
    self.m_loadOrder = order;

    for (const QString& id : self.m_loadOrder) {
        PluginSpec& spec = self.m_specs[id];

        IPlugin* plugin = spec.instantiate();
        if (!plugin) {
			if (self.m_loadersById.contains(id)) {
				if (QPluginLoader* loader = self.m_loadersById.value(id)) {
					const QString es = loader->errorString();
					if (!es.isEmpty())
						self.m_lastErrors.push_back(QString("Plugin '%1' loader error: %2").arg(id, es));
				}
			}
            self.m_lastErrors.push_back(QString("Failed to instantiate plugin '%1':\n%2")
                                        .arg(id, spec.errorString()));
            return false;
        }
		if (self.m_loadersById.contains(id))
			self.m_loaderOwnedPlugins.insert(plugin);

		self.m_plugins.push_back(plugin);

        const auto r = plugin->initialize(arguments, self);
        if (!r.ok) {
            QStringList msgs = r.errors;
            if (msgs.isEmpty())
                msgs.push_back("Unknown initialization error.");
            spec.addError(msgs.join('\n'));
            self.m_lastErrors.push_back(QString("Plugin '%1' initialize() failed:\n%2")
                                        .arg(id, spec.errorString()));
            return false;
        }

        spec.markInitialized();
    }

    for (const QString& id : self.m_loadOrder) {
        PluginSpec& spec = self.m_specs[id];
        if (auto* p = spec.plugin())
            p->extensionsInitialized(self);
    }

    return true;
}

void PluginManager::addObject(QObject* obj)
{
    if (!obj)
        qFatal("PluginManager::addObject called with null.");

    auto& self = instance();

    if (self.m_objects.contains(obj))
        qFatal("PluginManager::addObject called with same object twice.");

    self.m_objects.push_back(obj);
}

void PluginManager::removeObject(QObject* obj)
{
    auto& self = instance();
    const int idx = self.m_objects.indexOf(obj);
    if (idx < 0)
        qFatal("PluginManager::removeObject called with unknown object.");
    self.m_objects.removeAt(idx);
}

QObject* PluginManager::getObject(const QString& objectName)
{
    auto& self = instance();
    for (QObject* o : self.m_objects) {
        if (o && o->objectName() == objectName)
            return o;
    }
    return nullptr;
}

} // namespace ExtensionSystem
