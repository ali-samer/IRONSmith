#include "aieplugin/panels/AieNewDesignDialog.hpp"

#include <utils/DocumentBundle.hpp>
#include <utils/PathUtils.hpp>
#include <utils/filesystem/FileSystemUtils.hpp>

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonObject>
#include <QtCore/QStandardPaths>
#include <QtCore/QTimer>
#include <QtGui/QColor>
#include <QtGui/QPalette>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDialogButtonBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QFormLayout>

namespace Aie::Internal {

namespace {

using namespace Qt::StringLiterals;

const QString kDefaultName = u"Untitled"_s;
const QString kDeviceFamilyKey = u"aie/newDesign/deviceFamily"_s;
const QString kLocationKey = u"aie/newDesign/location"_s;
const QString kProjectRootKey = u"projectExplorer/rootPath"_s;

const QString kFamilyAieMl = u"aie-ml"_s;
const QString kFamilyAieMlV2 = u"aie-ml-v2"_s;

bool containsPathSeparators(const QString& text)
{
    return text.contains(u'/') || text.contains(u'\\');
}

QString defaultLocationForEnvironment(const Utils::Environment& env)
{
    QString location = env.setting(Utils::EnvironmentScope::Global, kLocationKey, QString()).toString();
    if (!location.isEmpty())
        return location;

    location = env.setting(Utils::EnvironmentScope::Global, kProjectRootKey, QString()).toString();
    if (!location.isEmpty())
        return location;

    const QString docs = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (!docs.isEmpty())
        return QDir(docs).filePath(QStringLiteral("IRONSmith"));

    return QDir::currentPath();
}

} // namespace

AieNewDesignDialog::AieNewDesignDialog(QWidget* parent)
    : Utils::FormDialog(parent)
    , m_env(makeEnvironment())
{
    setTitleText(QStringLiteral("New Design"));
    setModal(true);
    buildUi();
    loadDefaults();
    updatePreview();
    updateActions();

    QTimer::singleShot(0, this, [this]() {
        if (m_nameEdit) {
            m_nameEdit->setFocus(Qt::OtherFocusReason);
            m_nameEdit->selectAll();
        }
    });
}

Utils::Environment AieNewDesignDialog::makeEnvironment()
{
    Utils::EnvironmentConfig cfg;
    cfg.organizationName = QStringLiteral("IRONSmith");
    cfg.applicationName = QStringLiteral("IRONSmith");
    return Utils::Environment(cfg);
}

AieNewDesignDialog::Result AieNewDesignDialog::result() const
{
    return m_result;
}

void AieNewDesignDialog::buildUi()
{
    auto* form = formLayout();

    m_nameEdit = new QLineEdit(this);
    form->addRow(QStringLiteral("Name"), m_nameEdit);

    m_deviceFamilyCombo = new QComboBox(this);
    m_deviceFamilyCombo->addItem(QStringLiteral("AI Engine-ML"), kFamilyAieMl);
    m_deviceFamilyCombo->addItem(QStringLiteral("AI Engine-ML v2"), kFamilyAieMlV2);
    form->addRow(QStringLiteral("Device family"), m_deviceFamilyCombo);

    auto* locationRow = new QWidget(this);
    auto* locationLayout = new QHBoxLayout(locationRow);
    locationLayout->setContentsMargins(0, 0, 0, 0);
    locationLayout->setSpacing(8);
    m_locationEdit = new QLineEdit(locationRow);
    m_locationEdit->setReadOnly(true);
    m_chooseLocationButton = new QPushButton(QStringLiteral("Choose..."), locationRow);
    locationLayout->addWidget(m_locationEdit, 1);
    locationLayout->addWidget(m_chooseLocationButton);

    form->addRow(QStringLiteral("Location"), locationRow);

    m_helperLabel = new QLabel(this);
    m_helperLabel->setWordWrap(true);
    form->addRow(QString(), m_helperLabel);

    m_errorLabel = new QLabel(this);
    m_errorLabel->setWordWrap(true);
    QPalette errorPalette = m_errorLabel->palette();
    errorPalette.setColor(QPalette::WindowText, QColor(194, 59, 34));
    m_errorLabel->setPalette(errorPalette);
    m_errorLabel->setVisible(false);
    contentLayout()->addWidget(m_errorLabel);

    auto* buttons = buttonBox();
    buttons->setStandardButtons(QDialogButtonBox::Cancel);
    m_createButton = buttons->addButton(QStringLiteral("Create Design"), QDialogButtonBox::AcceptRole);
    m_createButton->setDefault(true);

    connect(m_chooseLocationButton, &QPushButton::clicked, this, &AieNewDesignDialog::chooseLocation);
    connect(m_nameEdit, &QLineEdit::textChanged, this, [this]() {
        updatePreview();
        updateActions();
    });
    connect(m_deviceFamilyCombo, &QComboBox::currentIndexChanged, this, &AieNewDesignDialog::updateActions);
    connect(buttons, &QDialogButtonBox::rejected, this, &AieNewDesignDialog::reject);
    connect(m_createButton, &QPushButton::clicked, this, &AieNewDesignDialog::handleCreate);
}

void AieNewDesignDialog::loadDefaults()
{
    m_nameEdit->setText(kDefaultName);

    const QString savedFamily = m_env.setting(Utils::EnvironmentScope::Global, kDeviceFamilyKey, kFamilyAieMl).toString();
    const int familyIndex = m_deviceFamilyCombo->findData(savedFamily);
    m_deviceFamilyCombo->setCurrentIndex(familyIndex >= 0 ? familyIndex : 0);

    const QString location = defaultLocationForEnvironment(m_env);
    m_locationEdit->setText(QDir::cleanPath(location));
}

void AieNewDesignDialog::saveDefaults()
{
    m_env.setSetting(Utils::EnvironmentScope::Global, kDeviceFamilyKey, deviceFamilyKey());
    m_env.setSetting(Utils::EnvironmentScope::Global, kLocationKey, m_locationEdit->text().trimmed());
}

void AieNewDesignDialog::updatePreview()
{
    const QString name = m_nameEdit->text().trimmed();
    const QString resolved = resolvedBundlePath(name);
    if (resolved.isEmpty()) {
        m_helperLabel->setText(QStringLiteral("Will create: -"));
        return;
    }

    const QString nativePath = QDir::toNativeSeparators(resolved);
    m_helperLabel->setText(QStringLiteral("Will create: %1").arg(nativePath));
}

void AieNewDesignDialog::updateActions()
{
    QString error;
    const bool ok = validateInputs(&error);
    setError(ok ? QString() : error);
    if (m_createButton)
        m_createButton->setEnabled(ok);
}

void AieNewDesignDialog::setError(const QString& message)
{
    if (!m_errorLabel)
        return;
    m_errorLabel->setVisible(!message.isEmpty());
    m_errorLabel->setText(message);
}

void AieNewDesignDialog::chooseLocation()
{
    const QString current = m_locationEdit->text().trimmed();
    const QString dir = QFileDialog::getExistingDirectory(this,
                                                          QStringLiteral("Choose Location"),
                                                          current);
    if (dir.isEmpty())
        return;
    m_locationEdit->setText(QDir::cleanPath(dir));
    updatePreview();
    updateActions();
}

bool AieNewDesignDialog::validateInputs(QString* error) const
{
    const QString name = m_nameEdit->text().trimmed();
    if (name.isEmpty()) {
        if (error)
            *error = QStringLiteral("Name cannot be empty.");
        return false;
    }

    if (containsPathSeparators(name)) {
        if (error)
            *error = QStringLiteral("Name cannot contain path separators.");
        return false;
    }

    const QString location = m_locationEdit->text().trimmed();
    if (location.isEmpty()) {
        if (error)
            *error = QStringLiteral("Location cannot be empty.");
        return false;
    }

    return true;
}

bool AieNewDesignDialog::ensureLocationExists(QString* error)
{
    const QString location = m_locationEdit->text().trimmed();
    if (location.isEmpty()) {
        if (error)
            *error = QStringLiteral("Location cannot be empty.");
        return false;
    }

    QDir dir(location);
    if (dir.exists())
        return true;

    if (!dir.mkpath(QStringLiteral("."))) {
        if (error)
            *error = QStringLiteral("Failed to create folder: %1").arg(location);
        return false;
    }

    return true;
}

QString AieNewDesignDialog::resolvedBundlePath(const QString& name) const
{
    const QString trimmed = name.trimmed();
    const QString location = m_locationEdit ? m_locationEdit->text().trimmed() : QString();
    if (trimmed.isEmpty() || location.isEmpty())
        return {};

    const QString candidate = QDir(location).filePath(trimmed);
    return Utils::DocumentBundle::normalizeBundlePath(candidate);
}

QString AieNewDesignDialog::deviceFamilyKey() const
{
    if (!m_deviceFamilyCombo)
        return kFamilyAieMl;
    return m_deviceFamilyCombo->currentData().toString();
}

AieNewDesignDialog::DeviceFamily AieNewDesignDialog::deviceFamilyValue() const
{
    const QString key = deviceFamilyKey();
    return key == kFamilyAieMlV2 ? DeviceFamily::AieMlV2 : DeviceFamily::AieMl;
}

AieNewDesignDialog::ConflictChoice AieNewDesignDialog::promptConflict(const QString& path)
{
    QMessageBox box(this);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(QStringLiteral("Design Already Exists"));
    box.setText(QStringLiteral("A design already exists at this location."));
    box.setInformativeText(QDir::toNativeSeparators(path));

    auto* replace = box.addButton(QStringLiteral("Replace"), QMessageBox::DestructiveRole);
    auto* createCopy = box.addButton(QStringLiteral("Create Copy"), QMessageBox::AcceptRole);
    auto* chooseDifferent = box.addButton(QStringLiteral("Choose Different Name"), QMessageBox::RejectRole);
    box.setDefaultButton(chooseDifferent);

    box.exec();
    if (box.clickedButton() == replace)
        return ConflictChoice::Replace;
    if (box.clickedButton() == createCopy)
        return ConflictChoice::CreateCopy;
    return ConflictChoice::ChooseDifferent;
}

QString AieNewDesignDialog::uniqueBundlePath(const QString& existingPath) const
{
    const QFileInfo info(existingPath);
    const QDir dir(info.absolutePath());
    const QString candidate = Utils::FileSystemUtils::duplicateName(dir, info.fileName());
    if (candidate.isEmpty())
        return {};
    return dir.filePath(candidate);
}

void AieNewDesignDialog::handleCreate()
{
    QString error;
    if (!validateInputs(&error)) {
        setError(error);
        return;
    }

    if (!ensureLocationExists(&error)) {
        setError(error);
        return;
    }

    const QString name = m_nameEdit->text().trimmed();
    QString bundlePath = resolvedBundlePath(name);
    if (bundlePath.isEmpty()) {
        setError(QStringLiteral("Unable to resolve bundle path."));
        return;
    }

    if (QFileInfo::exists(bundlePath)) {
        const ConflictChoice choice = promptConflict(bundlePath);
        if (choice == ConflictChoice::ChooseDifferent) {
            m_nameEdit->setFocus(Qt::OtherFocusReason);
            m_nameEdit->selectAll();
            return;
        }

        if (choice == ConflictChoice::CreateCopy) {
            const QString uniquePath = uniqueBundlePath(bundlePath);
            if (uniquePath.isEmpty()) {
                setError(QStringLiteral("Unable to generate a unique design name."));
                return;
            }
            bundlePath = uniquePath;
        } else {
            QFileInfo existing(bundlePath);
            bool removed = false;
            if (existing.isDir())
                removed = QDir(bundlePath).removeRecursively();
            else
                removed = QFile::remove(bundlePath);

            if (!removed) {
                setError(QStringLiteral("Failed to replace existing design."));
                return;
            }
        }
    }

    Utils::DocumentBundle::BundleInit init;
    init.name = QFileInfo(bundlePath).completeBaseName();
    init.program = QJsonObject{
        { QStringLiteral("deviceFamily"), deviceFamilyKey() }
    };
    init.design = QJsonObject{};

    const Utils::Result created = Utils::DocumentBundle::create(bundlePath, init);
    if (!created.ok) {
        setError(created.errors.isEmpty() ? QStringLiteral("Failed to create design.") : created.errors.join("\n"));
        return;
    }

    m_result.name = init.name;
    m_result.location = m_locationEdit->text().trimmed();
    m_result.bundlePath = bundlePath;
    m_result.deviceFamily = deviceFamilyValue();
    m_result.created = true;

    saveDefaults();
    accept();
}

} // namespace Aie::Internal
