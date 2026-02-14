#pragma once

#include <utils/EnvironmentQtPolicy.hpp>
#include <utils/ui/FormDialog.hpp>
QT_BEGIN_NAMESPACE
class QComboBox;
class QLabel;
class QLineEdit;
class QPushButton;
QT_END_NAMESPACE

namespace Aie::Internal {

class AieNewDesignDialog final : public Utils::FormDialog
{
    Q_OBJECT

public:
    enum class DeviceFamily : unsigned char {
        AieMl,
        AieMlV2
    };

    struct Result final {
        QString name;
        QString location;
        QString bundlePath;
        DeviceFamily deviceFamily = DeviceFamily::AieMl;
        bool created = false;
    };

    explicit AieNewDesignDialog(QWidget* parent = nullptr);

    Result result() const;

private:
    void buildUi();
    void loadDefaults();
    void saveDefaults();
    void updatePreview();
    void updateActions();
    void setError(const QString& message);

    void chooseLocation();
    void handleCreate();

    bool validateInputs(QString* error) const;
    bool ensureLocationExists(QString* error);
    QString resolvedBundlePath(const QString& name) const;
    QString deviceFamilyKey() const;
    DeviceFamily deviceFamilyValue() const;

    enum class ConflictChoice : unsigned char {
        Replace,
        CreateCopy,
        ChooseDifferent
    };
    ConflictChoice promptConflict(const QString& path);
    QString uniqueBundlePath(const QString& existingPath) const;

    static Utils::Environment makeEnvironment();

    Utils::Environment m_env;
    Result m_result;

    QLineEdit* m_nameEdit = nullptr;
    QComboBox* m_deviceFamilyCombo = nullptr;
    QLineEdit* m_locationEdit = nullptr;
    QPushButton* m_chooseLocationButton = nullptr;
    QLabel* m_helperLabel = nullptr;
    QLabel* m_errorLabel = nullptr;
    QPushButton* m_createButton = nullptr;
};

} // namespace Aie::Internal
