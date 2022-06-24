#pragma once
#include "CMOSSetting.h"
#include "MakoWrapper.h"
//#include "ConfigurationSystems/ConfigStream.h"
#include <qwidget.h>
#include <QMap>
#include <QTreeView>
#include <QStringList>
#include <QObject>
#include <QTimer>
#include <qitemdelegate.h>
#include <qstandarditemmodel.h>
#include <qstring.h>
#include <VimbaCPP/Include/Feature.h>
#include <VimbaCPP/Include/IFeatureObserver.h>
#include <VimbaCPP/Include/VimbaSystem.h>
#include <QSortFilterProxyModel>
#include <utility>

using AVT::VmbAPI::FeaturePtr;
using AVT::VmbAPI::CameraPtr;

class FeatureObserver : public QObject, public AVT::VmbAPI::IFeatureObserver
{
    Q_OBJECT
public:

    FeatureObserver(CameraPtr pCam);
    ~FeatureObserver(void);
    virtual void FeatureChanged(const FeaturePtr& feature);

signals:
    void setChangedFeature(const QString& sFeat, const QString& sValue, const bool& bIsWritable);
};
typedef SP_DECL(FeatureObserver) FeatureObserverPtr;


class ItemDelegate;
class SortFilterProxyModel;
class MultiCompleter;
class LineEditCompleter;
class IntSpinBox;
class QComboBox;
class QPushButton;
class QCheckBox;
class TickSlider;
class DoubleTickSlider;

class MakoSettingControl : public QTreeView
{
    Q_OBJECT
    public:
        MakoSettingControl(QWidget* parent = nullptr);
        ~MakoSettingControl();
        // initialize pcam, featureobserver and the qtreeview
        void initialize(QString sID, CameraPtr pCam, QWidget* parent = nullptr);
        // called from parent widget to assemble a full controller gui, just to ease the burden in the parent class
        void initializeWidget(QWidget* widget);
        void updateRegisterFeature();
        void updateUnRegisterFeature();
        void saveFeaturesToTextFile(const QString& sDestPathAndFileName) {};
        //void handleSavingConfig(ConfigStream& configFile, std::string delim);


        /*getter for m_model*/
        const QStandardItemModel* const controllerModel() { return m_Model; }

        template<typename dataType>
        dataType getFeatureValue(std::string feaName, std::string& errstr);
        template<typename dataType>
        void setFeatureValue(std::string feaName, dataType val, std::string& errstr);

        void setSettings(MakoSettings newSettings);
        void updateCurrentSettings();
        MakoSettings getCurrentSettings() { return currentSettings; }
        void setExpActive(bool active); // dedicated for toggling expActive from MakoCamera.h
        void setPicsPerRep(int picsperrep); // dedicated for setPicsPerRep from MakoCameraCore.h
        std::pair<VmbInt64_t, VmbInt64_t> getMaxImageSize();

    protected:
        void showEvent(QShowEvent* event);
        void hideEvent(QHideEvent* event);

    private:
        void initialzePreset(); // settings that are defined for all mako
        void mapInformation(const QString sName, const FeaturePtr featPtr);
        QString getFeatureNameFromModel(const QModelIndex& item) const;
        QString getFeatureNameFromMap(const QString& sDisplayName) const;
        FeaturePtr getFeaturePtrFromMap(const QString& sFeature);
        bool isFeatureWritable(const QString& sFeature);
        void showIt(const QModelIndex item, const QString& sWhat);

        void sortCategoryAndAttribute(const FeaturePtr& featPtr, QStandardItemModel* Model);
        bool findCategory(const QMap <QString, QString>& map, const QString& sName) const;
        unsigned int getGrandParentLevel(const QList<QStandardItem*>& items, const QString& sGrandParent) const;
        bool registerFeatureObserver(const QString& sFeatureName);
        void unregisterFeatureObserver(const QString& sFeatureName);
        void updateExpandedTreeValue(const FeaturePtr& featPtr, const QString& sName);


        void createCommandButton(const QModelIndex item);  /*  maps to VmbFeatureDataCommand */
        void createEnumComboBox(const QModelIndex item);  /*  maps to VmbFeatureDataEnum */
        void createIntSliderSpinBox(const QModelIndex item);  /*  maps to VmbFeatureDataInt */
        void createFloatSliderEditBox(const QModelIndex item);  /*  maps to VmbFeatureDataFloat */
        void createBooleanCheckBox(const QModelIndex item);  /*  maps to VmbFeatureDataBool */
        void createStringEditBox(const QModelIndex item);  /*  maps to VmbFeatureDataString */
        //void createLineEdit(const QModelIndex& item); /*  maps to register stuffs*/
        //void createLogarithmicSlider(const QModelIndex& item); /*  maps to Exposure*/
        void resetControl();
        
        bool initCurrentFeatureInt(const QString& name);
        void setIntegerValue(const int& nValue);
        void updateCurrentIntValue();
        bool initCurrentFeatureFloat(const QString& name);
        void setFloatingValue(const double& dValue);
        void updateCurrentFloatValue();
        void onIntSliderReleased();
        void onFloatSliderReleased();
        void AdjustOffscreenPosition(QPoint& position, QWidget& parentWidget);
        template<typename COMPOUND_WIDGET>
        void updateEditWidget(COMPOUND_WIDGET* w, const QVariant& value, bool bIsWritable);
        void updateWidget(const bool bIsWritable, const QVariant& value);

        bool eventFilter(QObject* object, QEvent* event);


    public slots:
        void closeControls(void);
        void onClicked(const QModelIndex& index);

    private slots:
        void onSetChangedFeature(const QString& sFeature, const QString& sValue, const bool& bIsWritable);
        void pollingFeaturesValue();

        void expand(const QModelIndex& index);
        void collapse(const QModelIndex& index);
        void updateColWidth();

        void onCmdButtonClick();
        void onIntSpinBoxClick();
        void onFloatEditFinished();
        void onIntSliderChanged(int nValue);
        void onFloatSliderChanged(double dValue);
        void onBoolCheckBoxClick(bool bValue);
        void onEnumComboBoxClick(const QString& sSelected);
        void onEditText();
        
    signals:
        void setDescription(const QString& sDesc);
        void acquisitionStartStop(const QString& sThisFeature);
        void updateStatusBar();
        void resetFPS();


    public:
        /* Filter Search Proxy */
        SortFilterProxyModel*               m_ProxyModel;
    private:
        MakoSettings                        currentSettings;


        FeaturePtrVector                    m_featPtrVec;
        CameraPtr                           m_pCam;
        MultiCompleter*                     m_StringCompleter;
        ItemDelegate*                       m_TreeDelegate;
        IFeatureObserverPtr                 m_pFeatureObs;
        QString                             m_sCameraID;
        // QMap<key, value>
        QMap <QString, QString>             m_DescriptonMap;
        QMap <QString, QString>             m_DisplayFeatureNameMap;
        QMap <QString, FeaturePtr>          m_featPtrMap;

        QVector < QMap <QString, QString> > m_Level;
        QMap <QString, QString>             m_Level0Map;
        QMap <QString, QString>             m_Level1Map;
        QMap <QString, QString>             m_Level2Map;
        QMap <QString, QString>             m_Level3Map;
        QMap <QString, QString>             m_Level4Map;
        QMap <QString, QString>             m_Level5Map;
        QMap <QString, QString>             m_Level6Map;
        QMap <QString, QString>             m_Level7Map;
        QMap <QString, QString>             m_Level8Map;
        QMap <QString, QString>             m_Level9Map;

        FeaturePtr                          m_FeaturePtr_Command;
        FeaturePtr                          m_FeaturePtr_EnumComboBox;
        FeaturePtr                          m_FeaturePtr_CheckBox;
        FeaturePtr                          m_FeaturePtr_IntSpinBox;
        FeaturePtr                          m_FeaturePtr_FloatSliderSpinBox;
        FeaturePtr                          m_FeaturePtr_StringEditBox;
        FeaturePtr                          m_FeaturePtr_LineEdit;

        QString                             m_sFeature_Command;
        QString                             m_sFeature_EnumComboBox;
        QString                             m_sFeature_CheckBox;
        QString                             m_sFeature_IntSpinBox;
        QString                             m_sFeature_FloatSliderSpinBox;
        QString                             m_sFeature_StringEditBox;
        QString                             m_sFeature_StringLineEdit;
        QString                             m_sCurrentSelectedFeature;

        QStandardItemModel*                 m_Model;

        int                                 m_nIntSliderOldValue;
        VmbInt64_t                          m_nMinimum;
        VmbInt64_t                          m_nMaximum;
        VmbInt64_t                          m_nIncrement;
        double                              m_dMinimum;
        double                              m_dMaximum;
        double                              m_dIncrement;
        StringVector                        m_sEnumEntries;

        /* Button */
        QPushButton*                        m_CmdButton;
        QWidget*                            m_ButtonWidget;

        /* ComboBox */
        QWidget*                            m_ComboWidget;
        QComboBox*                          m_EnumComboBox;

        /* Integer Feature Slider-SpinBoxes */
        QWidget*                            m_IntSpinSliderWidget;
        IntSpinBox*                         m_SpinBox_Int;
        QSlider*                            m_Slider_Int;

        /* Float Feature Slider-Edit */
        QWidget*                            m_FloatSliderEditWidget;
        QLineEdit*                          m_EditBox_Float;
        DoubleTickSlider*                   m_Slider_Float;

        /* String Feature EditBox */
        QWidget*                            m_EditWidget;
        QLineEdit*                          m_TextEdit_String;

        /* Boolean Feature CheckBox */
        QWidget*                            m_BooleanWidget;
        QCheckBox*                          m_CheckBox_Bool;


        bool                                m_bIsTooltipOn;
        QString                             m_sTooltip;

        bool                                m_bIsJobDone;
        bool                                m_bIsBusy;
        bool                                m_bIsMousePressed;

        QTimer*                             m_FeaturesPollingTimer;
        QList <QString>                     m_FeaturesPollingName;
        
};


