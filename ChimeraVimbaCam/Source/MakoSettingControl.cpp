#include "stdafx.h"
#define NOMINMAX
#include "MakoSettingControl.h"
//#include "ConfigurationSystems/ConfigSystem.h"
#include <LineEditCompleter.h>
#include <TickSlider.h>
#include <Helper.h>
#include <qlayout.h>
#include <qspinbox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qdesktopwidget.h>
#include <qpushbutton.h>
#include <qtextedit.h>
#include <qlabel.h>
#include <qsplitter.h>
#include <limits>

#ifndef WIN32
#include <cstring>
#endif


FeatureObserver::FeatureObserver(CameraPtr pCam)
{
    
}

FeatureObserver::~FeatureObserver(void)
{

}

void FeatureObserver::FeatureChanged(const AVT::VmbAPI::FeaturePtr& feature)
{
    if (feature != NULL)
    {
        std::string stdName("");

        VmbError_t error = feature->GetDisplayName(stdName);
        if (stdName.empty())
        {
            error = feature->GetName(stdName);
        }

        if (VmbErrorSuccess == error)
        {
            std::string sValue = MakoWrapper::getFeatureValue(feature);
            bool bIsWritable = false;
            if (VmbErrorSuccess == feature->IsWritable(bIsWritable))
            {
                emit setChangedFeature(qstr(stdName), qstr(sValue), bIsWritable);
            }
        }
    }
}


/* controlling row/height size in tree */
class ItemDelegate : public QItemDelegate
{
public:
    ItemDelegate() {}
    QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        QSize s = QItemDelegate::sizeHint(option, index);
        s.setHeight(row_height);
        return s;
    }

    void setRowHeight(const unsigned char height)
    {
        row_height = height;
    }

protected:
    unsigned char row_height;
};

//Description: Filter pattern proxy
class SortFilterProxyModel : public QSortFilterProxyModel
{
public:
    SortFilterProxyModel(QObject* parent = 0) : QSortFilterProxyModel(parent) {};

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const
    {
        // get source-model index for current row
        QModelIndex source_index = sourceModel()->index(sourceRow, this->filterKeyColumn(), sourceParent);
        if (source_index.isValid())
        {
            // if any of children matches the filter, then current index matches the filter as well
            int nCount = sourceModel()->rowCount(source_index);
            for (int i = 0; i < nCount; ++i)
            {
                // this is a recursive iteration untill it reaches the last layer
                if (filterAcceptsRow(i, source_index))
                {
                    return true;
                }
            }
            // check current index itself :
            QString key = sourceModel()->data(source_index, filterRole()).toString();
            return key.contains(filterRegExp());
        }

        // parent call for initial behaviour
        return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
    }

};

class IntSpinBox : public QSpinBox
{
    //Q_OBJECT
public:
    IntSpinBox(QWidget* parent) {};
    ~IntSpinBox() {};

protected:

private:
    virtual void stepBy(int steps) 
    {
        int nInterval = singleStep();
        int nValue = value();
        if (0 < steps) { //stepUp
            if (maximum() >= (nValue + nInterval)) {
                setValue(nValue + nInterval);
            }
        }
        else { //stepDown
            if (0 != (nValue - nInterval) % nInterval) {
                /* value % ninterval should be 0 */
                nValue -= ((nValue - nInterval) + (nInterval - ((nValue - nInterval) % nInterval)));
                if (minimum() <= nValue) {
                    setValue(nValue);
                }
            }
            else {
                setValue(nValue - nInterval);
            }
        }

        editingFinished();
    }
};



MakoSettingControl::MakoSettingControl(QWidget* parent)
    : QTreeView ( parent ), m_TreeDelegate ( nullptr ), 
    m_BooleanWidget         ( nullptr ), m_CheckBox_Bool   ( nullptr ),
    m_EditWidget            ( nullptr ), m_TextEdit_String ( nullptr ),
    m_ButtonWidget          ( nullptr ), m_CmdButton       ( nullptr ),
    m_ComboWidget           ( nullptr ), m_EnumComboBox    ( nullptr ),
    m_IntSpinSliderWidget   ( nullptr ), m_SpinBox_Int     ( nullptr ), m_Slider_Int   ( nullptr ),
    m_FloatSliderEditWidget ( nullptr ), m_EditBox_Float   ( nullptr ), m_Slider_Float ( nullptr ),
    m_FeaturesPollingTimer  ( nullptr ),
    m_dMinimum ( 0 ), m_dMaximum ( 0 ), m_dIncrement ( 0 ), m_nIntSliderOldValue ( 0 ), 
    m_nMinimum ( 0 ), m_nMaximum ( 0 ), m_nIncrement ( 0 ), 
    m_bIsTooltipOn ( true ), m_bIsJobDone ( true ), m_bIsMousePressed ( false ), m_bIsBusy ( false )
{
    m_pCam = SP_DECL(Camera)((Camera*)nullptr);
    m_pFeatureObs = SP_DECL(IFeatureObserver)((IFeatureObserver*)nullptr);
}

MakoSettingControl::~MakoSettingControl()
{
    if (m_FeaturesPollingTimer != nullptr) {
        m_FeaturesPollingTimer->stop();
    }
    resetControl();
    //m_pCam->Close();
}

void MakoSettingControl::initialize(QString sID, CameraPtr pCam, QWidget* parent)
{
    m_pCam = pCam;
    m_sCameraID = sID;
    m_pFeatureObs = SP_DECL(IFeatureObserver)(new FeatureObserver(pCam));
    VmbErrorType error;
    //setup Tree
    m_Model = new QStandardItemModel(this);
    QStringList sHeader;
    sHeader << tr("Feature ") << tr("Value ");
    m_Model->setHorizontalHeaderLabels(sHeader);

    m_Level.append(m_Level0Map);
    m_Level.append(m_Level1Map);
    m_Level.append(m_Level2Map);
    m_Level.append(m_Level3Map);
    m_Level.append(m_Level4Map);
    m_Level.append(m_Level5Map);
    m_Level.append(m_Level6Map);
    m_Level.append(m_Level7Map);
    m_Level.append(m_Level8Map);
    m_Level.append(m_Level9Map);

    /* Features string completer for filter */
    QStringList CompletionList;
    error = m_pCam->GetFeatures(m_featPtrVec);
    if (VmbErrorSuccess == error)
    {
        for (unsigned int i = 0; i < m_featPtrVec.size(); i++)
        {
            VmbFeatureVisibilityType visibilityType;
            m_featPtrVec.at(i)->GetVisibility(visibilityType);

            std::string sDisplayName;
            m_featPtrVec.at(i)->GetDisplayName(sDisplayName);

            /* Save DisplayName for filter completer */
            CompletionList << QString::fromStdString(sDisplayName);

            if ((VmbFeatureVisibilityBeginner == visibilityType) || (VmbFeatureVisibilityExpert == visibilityType) || (VmbFeatureVisibilityGuru == visibilityType))
            {
                sortCategoryAndAttribute(m_featPtrVec.at(i), m_Model);
                mapInformation(QString::fromStdString(sDisplayName), m_featPtrVec.at(i));
                m_featPtrMap[QString::fromStdString(sDisplayName)] = m_featPtrVec.at(i);
                std::string sFeatureName;
                m_featPtrVec.at(i)->GetName(sFeatureName);
                m_DisplayFeatureNameMap[QString::fromStdString(sDisplayName)] = QString::fromStdString(sFeatureName);
            }
        }
    }
    else {
        thrower("Mako getting Camera pointers failed, check whether camera is still connected."
            "\nthis error is very abnormal");
    }
    m_StringCompleter = new MultiCompleter(CompletionList, this);

    m_ProxyModel = new SortFilterProxyModel(this);
    m_ProxyModel->setDynamicSortFilter(true);
    setModel(m_ProxyModel);
    m_ProxyModel->setSourceModel(m_Model);

    resizeColumnToContents(0);
    /* used to set the maximum height of the row in tree */
    m_TreeDelegate = new ItemDelegate();
    m_TreeDelegate->setRowHeight(20);
    setItemDelegate(m_TreeDelegate);

    connect(this, SIGNAL(expanded(const QModelIndex&)), this, SLOT(expand(const QModelIndex&)));
    connect(this, SIGNAL(expanded(const QModelIndex&)), this, SLOT(updateColWidth()));
    connect(this, SIGNAL(collapsed(const QModelIndex&)), this, SLOT(collapse(const QModelIndex&)));
    connect((FeatureObserver*)(m_pFeatureObs.get()), &FeatureObserver::setChangedFeature,
        this, &MakoSettingControl::onSetChangedFeature);
    connect(this, &QTreeView::clicked, this, &MakoSettingControl::onClicked);

    sortByColumn(0, Qt::AscendingOrder);
    setSelectionMode(QAbstractItemView::SingleSelection);

    m_FeaturesPollingTimer = new QTimer(this);
    connect(m_FeaturesPollingTimer, &QTimer::timeout, this, &MakoSettingControl::pollingFeaturesValue);
    m_FeaturesPollingTimer->start(500);

    initialzePreset();
}

void MakoSettingControl::initialzePreset()
{
    std::string errstr("");
    std::string modeName = getFeatureValue<std::string>("DeviceModelName", errstr);
    if (modeName.find("G-319B") != std::string::npos || modeName.find("G-125B") != std::string::npos) { // this is a global shutter camera, does not have "SensorShutterMode" feature
    }
    else {
        setFeatureValue("SensorShutterMode", "GlobalReset", errstr);
        setFeatureValue("BinningHorizontalMode", "Sum", errstr);
        setFeatureValue("BinningVerticalMode", "Average", errstr);
    }
    setFeatureValue("AcquisitionMode", "Continuous", errstr);
    setFeatureValue("Gamma", 1.0, errstr); 
    setFeatureValue("PixelFormat", "Mono12", errstr);
    setFeatureValue("ExposureAuto", "Off", errstr);
    setFeatureValue("BlackLevel", 0.0, errstr);
    FeaturePtr gsvp = getFeaturePtrFromMap("GVSP Adjust Packet Size"); //adjust package size to make sure it works
    gsvp->RunCommand();
}

void MakoSettingControl::showEvent(QShowEvent* event)
{
    updateRegisterFeature();
    if (m_sCurrentSelectedFeature != "")
    {
        FeaturePtr pFeature = getFeaturePtrFromMap(m_sCurrentSelectedFeature);
        /* make sure to update the info */
        mapInformation(m_sCurrentSelectedFeature, pFeature);
        showIt(this->currentIndex(), "Description");
    }
}

void MakoSettingControl::hideEvent(QHideEvent* event)
{
    updateUnRegisterFeature();
    this->setToolTip("");
    emit setDescription("");
}

void MakoSettingControl::onSetChangedFeature(const QString& sFeature, const QString& sValue, const bool& bIsWritable) 
{
    QModelIndexList currentItems = m_Model->match(m_Model->index(0, 0), Qt::DisplayRole, QVariant::fromValue(sFeature), 1, Qt::MatchWrap | Qt::MatchRecursive);

    if (0 == currentItems.size())
        return;

    QString sFeatureValue = sValue;

    if (Helper::needsIPv4Format(sFeature))
    {
        sFeatureValue = Helper::displayValueToIPv4(sValue);
    }
    else if (sFeature == "Device MAC Address")
    {
        sFeatureValue = Helper::displayValueToMAC(sValue);
    }

    QModelIndex newIndex = m_Model->index(currentItems.at(0).row(), 1, currentItems.at(0).parent());
    m_Model->setData(newIndex, QVariant(sFeatureValue), Qt::EditRole);

    //TLParamsLock?
    QList<QStandardItem*> itemsCol1 = m_Model->findItems(sFeature, Qt::MatchWrap | Qt::MatchRecursive, 0);

    if (!itemsCol1.empty())
    {
        bIsWritable ? itemsCol1.at(0)->setForeground(QColor(0, 128, 0))/*green*/ : itemsCol1.at(0)->setForeground(QColor(135, 135, 135))/*grey*/;
        if (0 == m_sCurrentSelectedFeature.compare(sFeature))
            updateWidget(bIsWritable, QVariant(sFeatureValue));
    }

    if (!m_FeaturePtr_EnumComboBox || !m_EnumComboBox)
        return;

    /* update Exposure-/Gain-/BalanceWhite- Auto when "Once" clicked */
    if (Helper::isAutoFeature(m_sCurrentSelectedFeature)
        && Helper::isAutoFeature(sFeature)
        && m_sCurrentSelectedFeature == sFeature
        && sFeatureValue == "Off"
        && m_EnumComboBox->currentText() == "Once")
    {
        const int nIndex = m_EnumComboBox->findText("Off");
        if (-1 != nIndex)
            m_EnumComboBox->setCurrentIndex(nIndex);
    }

    if (m_sCurrentSelectedFeature == sFeature && sValue.isEmpty())
    {
        m_EnumComboBox->setItemText(m_EnumComboBox->count(), sValue);
        m_EnumComboBox->setCurrentIndex(m_EnumComboBox->count());
    }
}

void MakoSettingControl::pollingFeaturesValue()
{
    for (int i = 0; i < m_FeaturesPollingName.size(); i++)
    {
        FeaturePtr FeatPtr = getFeaturePtrFromMap(m_FeaturesPollingName.at(i));

        VmbUint32_t nPollValue = 0;
        if (VmbErrorSuccess == FeatPtr->GetPollingTime(nPollValue))
        {
            if (0 == nPollValue)
            {
                updateExpandedTreeValue(FeatPtr, m_FeaturesPollingName.at(i));
            }
        }
    }
}

void MakoSettingControl::mapInformation(const QString sName, const FeaturePtr featPtr)
{
    if (nullptr == featPtr)
        return;

    std::string sDescription;
    featPtr->GetDescription(sDescription);

    QString sQDescription = "<br/>";
    sQDescription += qstr(sDescription);
    sQDescription = sQDescription.replace(".", ".<br/>");
    
    std::string sInfo = MakoWrapper::getFeatureInformation(featPtr);
    if (sInfo.empty())
    {
        m_DescriptonMap[sName] = sQDescription;
    }
    else
    {
        if (sDescription.empty())
            m_DescriptonMap[sName] = "<b>DESCRIPTION:</b> N/A<br/>" + qstr(sInfo);
        else
            m_DescriptonMap[sName] = "<b>DESCRIPTION:</b>" + sQDescription + "<br/>" + qstr(sInfo);
    }
}

void MakoSettingControl::onClicked(const QModelIndex& current)
{
    setCursor(Qt::ArrowCursor);
    scrollTo(current, QAbstractItemView::EnsureVisible);
    if (m_bIsBusy)
        return;
    m_bIsBusy = true;

    if (!current.isValid())
    {
        m_bIsBusy = false;
        return;
    }

    /* check what feature is that */
    FeaturePtr FeatPtr;
    QString sFeature = getFeatureNameFromModel(current);
    FeatPtr = getFeaturePtrFromMap(sFeature);

    /* make sure to update the info */
    mapInformation(sFeature, FeatPtr);
    showIt(current, "Description");

    if ((nullptr == FeatPtr) || !isFeatureWritable(sFeature) || 0 == current.column())
    {
        m_bIsBusy = false;
        m_sCurrentSelectedFeature = sFeature;
        resetControl();
        return;
    }

    VmbFeatureDataType dataType = VmbFeatureDataUnknown;
    if (VmbErrorSuccess == FeatPtr->GetDataType(dataType))
    {
        switch (dataType)
        {
        case VmbFeatureDataInt:     createIntSliderSpinBox(current);   break;
        case VmbFeatureDataFloat:   createFloatSliderEditBox(current); break;
        case VmbFeatureDataEnum:    createEnumComboBox(current);       break;
        case VmbFeatureDataString:  createStringEditBox(current);      break;
        case VmbFeatureDataBool:    createBooleanCheckBox(current);    break;
        case VmbFeatureDataCommand: createCommandButton(current);      break;
        case VmbFeatureDataRaw:     /*createHexEditor(current);*/      break;
        default: break;
        }
    }

    m_bIsBusy = false;
}

void MakoSettingControl::collapse(const QModelIndex& index)
{
    QVariant varData = index.data();
    int nrow = 0;
    QVariant varChild = 1;

    while (0 != varChild.type())
    {
        QModelIndex child = index.child(nrow++, 0);

        if (0 == child.data().type())
            break;

        varChild = child.data();
        if (!MakoWrapper::isEventFeature(getFeaturePtrFromMap(varChild.toString())))
        {
            unregisterFeatureObserver(varChild.toString());
            for (int i = 0; i < m_FeaturesPollingName.size(); i++)
            {
                if (m_FeaturesPollingName.at(i) == varChild.toString())
                {
                    m_FeaturesPollingName.removeAt(i);
                }
            }
        }
        collapse(child);
    }
}

void MakoSettingControl::expand(const QModelIndex& index)
{
    QVariant varData = index.data();
    int nrow = 0;
    QVariant varChild = 1;

    while (0 != varChild.type())
    {
        QModelIndex child = index.child(nrow++, 0);
        if (0 == child.data().type())
            break;
        varChild = child.data();

        if (isExpanded(child)) // Returns true if the model item index is expanded; otherwise returns false.
            expand(child); // this is a recursive iteration of calling 'expand' itself

        FeaturePtr pFeature = getFeaturePtrFromMap(varChild.toString());

        if (!MakoWrapper::isEventFeature(pFeature))
        {
            if (!SP_ISNULL(pFeature))
            {
                VmbFeatureFlagsType flags = (VmbFeatureFlagsType)0UL;
                VmbUint32_t         pollingTime = 0;
                if (VmbErrorSuccess == pFeature->GetFlags(flags)
                    && VmbFeatureFlagsVolatile & flags
                    && VmbErrorSuccess == pFeature->GetPollingTime(pollingTime)
                    && 0 == pollingTime)
                    m_FeaturesPollingName.push_back(varChild.toString());
                else
                    registerFeatureObserver(varChild.toString());
            }
        }
    }
}


void MakoSettingControl::updateColWidth()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
}

void MakoSettingControl::initializeWidget(QWidget* widget)
{
    if (widget == nullptr) {
        return;
    }
    MakoSettingControl* controller = this;
    QVBoxLayout* ctrlDiaVLayout = new QVBoxLayout(widget);
    ctrlDiaVLayout->setContentsMargins(0, 0, 0, 0);
    widget->setLayout(ctrlDiaVLayout);

    /* add Filter Pattern */
    QHBoxLayout* pattern_HLayout = new QHBoxLayout(widget);
    QLabel* filterPatternLabel = new QLabel("Filter pattern:", widget);
    filterPatternLabel->setStyleSheet("font-weight: bold;");
    LineEditCompleter* filterPatternLineEdit = new LineEditCompleter(widget);
    filterPatternLineEdit->setText(tr("Example: Gain|Width"));
    filterPatternLineEdit->setToolTip(tr("To filter multiple features, e.g: Width|Gain|xyz|etc"));
    filterPatternLineEdit->setCompleter(controller->m_StringCompleter);
    filterPatternLineEdit->setMinimumWidth(200);
    QPushButton* patternButton = new QPushButton("Search", widget);

    /*put controller related into controller dialog*/
    pattern_HLayout->addWidget(filterPatternLabel);
    pattern_HLayout->addWidget(filterPatternLineEdit);
    pattern_HLayout->addWidget(patternButton);

    // this widget holds the Hlayout(filterEdit) and controller Tree
    QWidget* ctrlTreeVerticalLayoutWidget = new QWidget(widget);
    QVBoxLayout* ctrlTreeVerticalLayout = new QVBoxLayout(widget);
    ctrlTreeVerticalLayout->addLayout(pattern_HLayout, 0);
    ctrlTreeVerticalLayout->addWidget(controller, 1);
    ctrlTreeVerticalLayout->setContentsMargins(0, 0, 0, 0);
    ctrlTreeVerticalLayoutWidget->setLayout(ctrlTreeVerticalLayout);

    QTextEdit* description = new QTextEdit(widget);
    description->setLineWrapMode(QTextEdit::NoWrap);
    description->setReadOnly(true);
    description->setStyleSheet("font: 12px;\n" "font-family: Verdana;\n");

    // the splitter holds ctrlTreeVerticalLayoutWidget and description QTextEdit
    QSplitter* splitter = new QSplitter(widget);
    QList<int> listSize;
    listSize << 5000;
    splitter->setChildrenCollapsible(false);
    splitter->setOrientation(Qt::Vertical);
    splitter->addWidget(ctrlTreeVerticalLayoutWidget);
    splitter->addWidget(description);
    splitter->setSizes(listSize);

    ctrlDiaVLayout->addWidget(splitter);

    connect(this, &MakoSettingControl::setDescription, [this, description](const QString& descrip) {
        description->setText(descrip); });
    connect(filterPatternLineEdit, &QLineEdit::returnPressed, [this, filterPatternLineEdit]() {
        QRegExp::PatternSyntax syntax = QRegExp::PatternSyntax(0);
        Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
        QRegExp regExp(filterPatternLineEdit->text(), caseSensitivity, syntax);
        this->m_ProxyModel->setFilterRegExp(regExp);
        this->expandAll();
        this->updateUnRegisterFeature();
        this->updateRegisterFeature();
        filterPatternLineEdit->setFocus();
        filterPatternLineEdit->selectAll(); });
    connect(patternButton, &QPushButton::clicked, [this, filterPatternLineEdit](bool checked) {
        QRegExp::PatternSyntax syntax = QRegExp::PatternSyntax(0);
        Qt::CaseSensitivity caseSensitivity = Qt::CaseInsensitive;
        QRegExp regExp(filterPatternLineEdit->text(), caseSensitivity, syntax);
        this->m_ProxyModel->setFilterRegExp(regExp);
        this->expandAll();
        this->updateUnRegisterFeature();
        this->updateRegisterFeature();
        filterPatternLineEdit->setFocus();
        filterPatternLineEdit->selectAll(); });


}

void MakoSettingControl::updateRegisterFeature()
{
    try {
        for (int i = 0; i < model()->rowCount(); i++)
        {
            if (isExpanded(model()->index(i, 0)))
            {
                expand(model()->index(i, 0));
            }
        }

        updateColWidth();
    }
    catch (ChimeraError& e) {
        errBox(e.trace());
    }

}

void MakoSettingControl::updateUnRegisterFeature()
{
    for (int i = 0; i < model()->rowCount(); i++)
    {
        collapse(model()->index(i, 0));
    }
}

//void MakoSettingControl::handleSavingConfig(ConfigStream& configFile, std::string delim)
//{
//    if (m_pCam == nullptr) {
//        /*return*/; //means in SAFEMODE, do not update the setting but do write to the config
//    }
//    else {
//        updateCurrentSettings();
//    }
//    configFile << delim + "\n";
//    configFile << "/*Mako System Active:*/ " << currentSettings.expActive
//        << "\n/*Left:*/ " << currentSettings.dims.left
//        << "\n/*Top:*/ " << currentSettings.dims.top
//        << "\n/*Right:*/ " << currentSettings.dims.right
//        << "\n/*Bottom:*/ " << currentSettings.dims.bottom
//        << "\n/*H-Bin:*/ " << currentSettings.dims.horizontalBinning
//        << "\n/*V-Bin:*/ " << currentSettings.dims.verticalBinning
//        << "\n/*Exposure Time:*/ " << currentSettings.exposureTime
//        << "\n/*Frame Rate:*/ " << currentSettings.frameRate
//        << "\n/*Raw Gain:*/ " << currentSettings.rawGain
//        << "\n/*Pics Per Rep:*/ " << currentSettings.picsPerRep
//        << "\n/*Trigger Mode:*/ " << MakoTrigger::toStr(currentSettings.triggerMode)
//        << "\n";
//    configFile << "END_"+ delim + "\n";
//}

QString MakoSettingControl::getFeatureNameFromModel(const QModelIndex& item) const
{
    QString sAttrName("");
    if (item.isValid())
    {
        QModelIndex indexFirstCol = item.sibling(item.row(), 0);
        sAttrName = indexFirstCol.model()->data(indexFirstCol, Qt::DisplayRole).toString();
    }
    return sAttrName;
}

FeaturePtr MakoSettingControl::getFeaturePtrFromMap(const QString& sFeature)
{
    QMap<QString, FeaturePtr>::iterator i = m_featPtrMap.find(sFeature);

    if (m_featPtrMap.end() != i)
    {
        return i.value();
    }
    else
    {
        return FeaturePtr();
    }
}
QString MakoSettingControl::getFeatureNameFromMap(const QString& sDisplayName) const
{
    QString sFeature;
    QMap<QString, QString>::const_iterator itr = m_DisplayFeatureNameMap.find(sDisplayName);
    if (itr != m_DisplayFeatureNameMap.constEnd())
    {
        sFeature = itr.value();
    }
    return sFeature;
}

bool MakoSettingControl::isFeatureWritable(const QString& sFeature)
{
    bool bIsWritable = false;
    FeaturePtr FeatPtr;

    FeatPtr = getFeaturePtrFromMap(sFeature);
    if (FeaturePtr() == FeatPtr)
        return bIsWritable;

    FeatPtr->IsWritable(bIsWritable);

    return bIsWritable;
}

void MakoSettingControl::showIt(const QModelIndex item, const QString& sWhat)
{
    QString sAttrName = getFeatureNameFromModel(item);

    QMap<QString, QString>::const_iterator i;

    if (0 == sWhat.compare("Description"))
        i = m_DescriptonMap.find(sAttrName);

    while (i != m_DescriptonMap.constEnd())
    {
        if (0 == i.key().compare(sAttrName))
        {
            m_sTooltip = i.value();
            if (m_bIsTooltipOn)
            {
                this->setToolTip(i.value());
            }
            else
                this->setToolTip("");

            emit setDescription(i.value());
            return;
        }
        ++i;
    }
    /* no feature description has been found */
    this->setToolTip("");
    emit setDescription("");
}

bool  MakoSettingControl::findCategory(const QMap <QString, QString>& map, const QString& sName) const
{
    QMap<QString, QString>::const_iterator itr = map.find(sName);
    while (itr != map.constEnd())
    {
        if (0 == itr.key().compare(sName))
        {
            return true;
        }

        ++itr;
    }

    return false;
}

void MakoSettingControl::sortCategoryAndAttribute(const FeaturePtr& featPtr, QStandardItemModel* Model)
{
    QFont categoryFont;
    categoryFont.setBold(true);

    QList<QStandardItem*> items;
    QList<QStandardItem*> pParent;
    QString sLastNode;
    bool bHasLastNode = false;
    int nLevel = 0;

    std::string sCategory, sFeatureName;
    std::string sFeatureValue;
    bool bIsWritable = false;
    VmbFeatureVisibilityType visibilityType;
    VmbError_t error;

    error = featPtr->GetDisplayName(sFeatureName);

    if (VmbErrorSuccess == featPtr->GetCategory(sCategory) &&
        VmbErrorSuccess == error)
    {
        /* TODO check return value */
        sFeatureValue = MakoWrapper::getFeatureValue(featPtr);
        error = featPtr->IsWritable(bIsWritable);
        featPtr->GetVisibility(visibilityType);

        /* Feature directly in root category */
        if (0 == sCategory.length())
        {
            sCategory = "/";
        }
    }
    else
    {
        return;
    }

    QString sPath = qstr(sCategory);

    int nCount = sPath.count('/');

    while ((nLevel < nCount))
    {
        nLevel++;
        QString sPart = sPath.section('/', nLevel, nLevel);
        /* Feature directly in root category */
        if (0 == sPart.length())
        {
            sPart = "/";
        }
        bool bNodeExists = false;
        if (0 < sPart.length())
        {
            /* check if category in level already available */
            if (!findCategory(m_Level.at(nLevel - 1), sPart))
            {
                bNodeExists = false;
                switch (nLevel - 1)
                {
                case 0: m_Level0Map[sPart] = "true";  /* TOP LEVEL */
                    m_Level[0] = m_Level0Map;
                    break;
                case 1:m_Level1Map[sPart] = "true";
                    m_Level[1] = m_Level1Map;
                    break;
                case 2:m_Level2Map[sPart] = "true";
                    m_Level[2] = m_Level2Map;
                    break;
                case 3:m_Level3Map[sPart] = "true";
                    m_Level[3] = m_Level3Map;
                    break;
                case 4:m_Level4Map[sPart] = "true";
                    m_Level[4] = m_Level4Map;
                    break;
                case 5:m_Level5Map[sPart] = "true";
                    m_Level[5] = m_Level5Map;
                    break;
                case 6:m_Level6Map[sPart] = "true";
                    m_Level[6] = m_Level6Map;
                    break;
                case 7:m_Level7Map[sPart] = "true";
                    m_Level[7] = m_Level7Map;
                    break;
                case 8:m_Level8Map[sPart] = "true";
                    m_Level[8] = m_Level8Map;
                    break;
                }
            }
            else
            {
                bNodeExists = true;
            }

            /* if category/feature not available in tree yet, then fill it */
            if (!bNodeExists)
            {
                QList<QStandardItem*> currentItem;

                currentItem << new QStandardItem(sPart) << new QStandardItem("");
                currentItem.at(0)->setEditable(false);
                currentItem.at(1)->setEditable(false);

                if (bHasLastNode)
                {
                    /* get the pointer of list items */
                    items = Model->findItems(sLastNode, Qt::MatchRecursive);
                    if (0 != items.size())
                    {
                        /* find the right parent level, and add the attribute item there*/
                        QString sGrandParent = sPath.section('/', nLevel - 1, nLevel - 1);
                        items.at(getGrandParentLevel(items, sGrandParent))->appendRow(currentItem);
                        sLastNode = sPart;
                        continue;
                    }
                }

                if (pParent.isEmpty())
                {
                    pParent << new QStandardItem(sPart) << new QStandardItem("");
                    pParent.at(0)->setFont(categoryFont);
                    pParent.at(0)->setEditable(false);
                    pParent.at(1)->setEditable(false);

                    Model->appendRow(pParent);

                    sLastNode = sPart;
                    continue;
                }

                /* use the last node as parent if available */
                items = Model->findItems(sLastNode, Qt::MatchRecursive);
                if (0 != items.size())
                {
                    /* find the right parent level, and add the attribute item there*/
                    QString sGrandParent = sPath.section('/', nLevel - 1, nLevel - 1);
                    items.at(getGrandParentLevel(items, sGrandParent))->appendRow(currentItem);
                }
                else
                {
                    pParent.at(0)->appendRow(currentItem);
                }
            }
            /* if already available use it as parent node */
            else
            {
                bHasLastNode = true;
            }

            sLastNode = sPart;
        }
    }

    /* get the pointer of list items */
    items = Model->findItems(sLastNode, Qt::MatchWrap | Qt::MatchRecursive);

    if (0 != items.size() && (VmbFeatureVisibilityInvisible != visibilityType))
    {
        QList<QStandardItem*> attributeItems;

        std::string sStdName;
        featPtr->GetName(sStdName);
        QString sName = QString::fromStdString(sStdName);
        std::string sRepresentation;
        featPtr->GetRepresentation(sRepresentation);

        if (Helper::needsIPv4Format(sName, sRepresentation))
        {
            sFeatureValue = str(Helper::displayValueToIPv4(qstr(sFeatureValue)));
        }

        attributeItems << new QStandardItem(QString::fromStdString(sFeatureName)) << new QStandardItem(qstr(sFeatureValue));

        attributeItems.at(0)->setEditable(false);
        attributeItems.at(1)->setEditable(false);

        bIsWritable ? attributeItems.at(0)->setForeground(QColor(0, 128, 0))/* green */ : attributeItems.at(0)->setForeground(QColor(135, 135, 135)) /* grey */;

        /* find the right parent level, and add the attribute item there*/
        QString sGrandParent = sPath.section('/', nCount - 1, nCount - 1);
        items.at(getGrandParentLevel(items, sGrandParent))->appendRow(attributeItems);

        /* register Events */
        if (MakoWrapper::isEventFeature(featPtr))
        {
            featPtr->RegisterObserver(m_pFeatureObs);
        }
    }
}

unsigned int MakoSettingControl::getGrandParentLevel(const QList<QStandardItem*>& items, const QString& sGrandParent) const
{
    unsigned int nLevel = 0;
    for (int i = 0; i < items.size(); i++)
    {
        QStandardItem* parent = items.at(i)->parent();

        if (0 == parent)
            continue;

        if (0 == sGrandParent.compare(parent->text()))
        {
            nLevel = i;
            break;
        }
    }
    return nLevel;
}

bool MakoSettingControl::registerFeatureObserver(const QString& sFeatureName)
{
    /* register all features to the observer, so you will get notification in case any features value has been changed */
    QMap<QString, FeaturePtr>::iterator find_pos = m_featPtrMap.find(sFeatureName);
    if (find_pos != m_featPtrMap.end())
    {
        VmbError_t error = find_pos.value()->RegisterObserver(m_pFeatureObs);
        if (VmbErrorSuccess != error)
        {
            thrower("ERROR Register Observer - Feature: " + str(sFeatureName) + " returned " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
        }
        updateExpandedTreeValue(find_pos.value(), sFeatureName);
        return true;
    }
    return false;
}

void MakoSettingControl::unregisterFeatureObserver(const QString& sFeatureName)
{
    QMap<QString, FeaturePtr>::iterator find_pos = m_featPtrMap.find(sFeatureName);

    if (find_pos != m_featPtrMap.end())
    {
        VmbError_t error = find_pos.value()->UnregisterObserver(m_pFeatureObs);
        if (VmbErrorSuccess != error)
        {
            if (VmbErrorNotFound != error)
                thrower("ERROR Unregister Observer - Feature: " + str(find_pos.key()) + " returned " + str(error) + " " +
                    str(Helper::mapReturnCodeToString(error)));
        }
    }
}

void MakoSettingControl::updateExpandedTreeValue(const FeaturePtr& featPtr, const QString& sName)
{
    std::string sValue = MakoWrapper::getFeatureValue(featPtr);
    bool bIsWritable = false;
    if (VmbErrorSuccess == featPtr->IsWritable(bIsWritable))
        onSetChangedFeature(sName, qstr(sValue), bIsWritable);
}

/***************************creat boxes for control**************************************/
/* VmbFeatureDataCommand */
void MakoSettingControl::createCommandButton(const QModelIndex item)
{
    QString sFeature_Command = getFeatureNameFromModel(item);

    if (isFeatureWritable(sFeature_Command))
    {
        m_sCurrentSelectedFeature = m_sFeature_Command = sFeature_Command;

        m_FeaturePtr_Command = getFeaturePtrFromMap(sFeature_Command);
        if (FeaturePtr() == m_FeaturePtr_Command)
            return;

        resetControl();

        m_ButtonWidget = new QWidget(this);
        QHBoxLayout* HButtonLayout = new QHBoxLayout(m_ButtonWidget);
        m_CmdButton = new QPushButton(QString("Execute"), m_ButtonWidget);

        QHBoxLayout* HButtonLayout2 = new QHBoxLayout(m_ButtonWidget);
        HButtonLayout2->addWidget(m_CmdButton);
        HButtonLayout->addLayout(HButtonLayout2);

        QPoint p = QCursor::pos();
        m_ButtonWidget->setFixedSize(m_sCurrentSelectedFeature.length() * 10, 60);
        m_ButtonWidget->setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint);
        m_ButtonWidget->setWindowTitle(m_sCurrentSelectedFeature);
        AdjustOffscreenPosition(p, *m_ButtonWidget);
        m_ButtonWidget->move(p.x(), p.y());
        m_ButtonWidget->show();

        QObject::connect(m_CmdButton, &QPushButton::clicked, this, &MakoSettingControl::onCmdButtonClick);
        setCursor(Qt::PointingHandCursor);
    }
}

/* run command when feature button clicked */
void MakoSettingControl::onCmdButtonClick()
{
    VmbError_t error;

    if ((0 == m_sFeature_Command.compare("AcquisitionStop"))
        || (0 == m_sFeature_Command.compare("Acquisition Stop"))
        || (0 == m_sFeature_Command.compare("AcquisitionAbort"))
        || (0 == m_sFeature_Command.compare("Acquisition Abort")))
    {
        emit acquisitionStartStop("AcquisitionStop");
    }
    else if ((0 == m_sFeature_Command.compare("AcquisitionStart"))
        || (0 == m_sFeature_Command.compare("Acquisition Start")))
    {
        emit acquisitionStartStop("AcquisitionStart");
    }
    else if ((0 != m_sFeature_Command.compare("AcquisitionStop"))
        && (0 != m_sFeature_Command.compare("Acquisition Stop"))
        && (0 != m_sFeature_Command.compare("AcquisitionAbort"))
        && (0 != m_sFeature_Command.compare("Acquisition Abort")))
    {
        error = m_FeaturePtr_Command->RunCommand();
        if (VmbErrorSuccess != error)
        {
            thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " returned " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
        }
    }

    if (0 == m_sFeature_Command.compare("GVSP Adjust Packet Size"))
    {
        bool bIsDone = false;
        this->setEnabled(false);

        while ((!bIsDone) && (VmbErrorSuccess == error))
        {
            error = m_FeaturePtr_Command->IsCommandDone(bIsDone);
        }
        this->setEnabled(true);
    }
    emit updateStatusBar();
}

/* VmbFeatureDataEnum */
void MakoSettingControl::createEnumComboBox(const QModelIndex item)
{
    VmbError_t error;
    QString sFeature_EnumComboBox = getFeatureNameFromModel(item);

    // Don't create drop down for read only features
    if (isFeatureWritable(sFeature_EnumComboBox))
    {
        m_sCurrentSelectedFeature = m_sFeature_EnumComboBox = sFeature_EnumComboBox;

        // Set FeaturePtr member to currently selected enum feature
        m_FeaturePtr_EnumComboBox = getFeaturePtrFromMap(m_sFeature_EnumComboBox);
        if (FeaturePtr() == m_FeaturePtr_EnumComboBox)
            return;
        // Get all possible enumerations as string
        m_sEnumEntries.clear();
        error = m_FeaturePtr_EnumComboBox->GetValues(m_sEnumEntries);
        resetControl();

        m_ComboWidget = new QWidget(this);
        QHBoxLayout* HComboLayout = new QHBoxLayout(m_ComboWidget);
        m_EnumComboBox = new QComboBox(m_ComboWidget);
        QHBoxLayout* HComboLayout2 = new QHBoxLayout(m_ComboWidget);
        HComboLayout2->addWidget(m_EnumComboBox);
        HComboLayout->addLayout(HComboLayout2);

        if (VmbErrorSuccess == error)
        {
            std::string sCurrentValue;
            unsigned int nPos = 0;
            // Get currently selected enumeration
            if (VmbErrorSuccess == m_FeaturePtr_EnumComboBox->GetValue(sCurrentValue) || VmbErrorSuccess == error)
            {
                for (unsigned int i = 0; i < m_sEnumEntries.size(); i++)
                {
                    // Check each enumeration if it is currently applicable
                    bool bIsAvailable = false;
                    m_FeaturePtr_EnumComboBox->IsValueAvailable(m_sEnumEntries.at(i).c_str(), bIsAvailable);
                    if (bIsAvailable)
                    {
                        // Add all applicable enumerations to drop down
                        m_EnumComboBox->addItem(qstr(m_sEnumEntries.at(i)));
                        // Is this currently selected enumeration?
                        if (0 == m_sEnumEntries.at(i).compare(sCurrentValue))
                            // Set drop down index
                            m_EnumComboBox->setCurrentIndex(nPos);
                        nPos++;
                    }
                }

                if (sCurrentValue.empty())
                {
                    m_EnumComboBox->setItemText(nPos, qstr(sCurrentValue));
                    m_EnumComboBox->setCurrentIndex(nPos);
                }
            }

            QPoint p = QCursor::pos();
            m_ComboWidget->setFixedSize(m_sCurrentSelectedFeature.length() * 13, 60);
            m_ComboWidget->setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint);
            m_ComboWidget->setWindowTitle(m_sCurrentSelectedFeature);
            AdjustOffscreenPosition(p, *m_ComboWidget);
            m_ComboWidget->move(p.x(), p.y());
            m_ComboWidget->show();

            QObject::connect(m_EnumComboBox, qOverload<const QString&>(&QComboBox::activated), this, &MakoSettingControl::onEnumComboBoxClick);
            setCursor(Qt::PointingHandCursor);
        }
        else
        {
            thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (GetEnumRange) returned " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
        }
    }
}

void MakoSettingControl::onEnumComboBoxClick(const QString& sSelected)
{
    std::string sCurrentValue;
    VmbError_t error;
    int nIndex = 0;
    int nIndexOld = 0;

    error = m_FeaturePtr_EnumComboBox->GetValue(sCurrentValue);
    if (VmbErrorSuccess == error)
    {
        nIndexOld = m_EnumComboBox->findText(qstr(sCurrentValue));
    }
    else
    {
        thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (GetEnumValue) " + str(sSelected) + " returned " + str(error) +
            " " + str(Helper::mapReturnCodeToString(error)));
    }

    if ((0 == m_sFeature_EnumComboBox.compare("PixelFormat")) || 
        (0 == m_sFeature_EnumComboBox.compare("Pixel Format")) ||
        (0 == m_sFeature_EnumComboBox.compare("AcquisitionMode")) || 
        (0 == m_sFeature_EnumComboBox.compare("Acquisition Mode")))
    {
        /* stop Acquisition */
        emit acquisitionStartStop("AcquisitionStop");

        /* give a bit time to make sure that camera is stopped */
        #ifdef WIN32
        ::Sleep(5);
        #else
        ::usleep(5000);
        #endif

        error = m_FeaturePtr_EnumComboBox->SetValue(sSelected.toUtf8().data());
        if (VmbErrorSuccess != error)
        {
            thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (SetEnumValue) " + str(sSelected) + " returned " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
        }

        if (VmbErrorSuccess == m_FeaturePtr_EnumComboBox->GetValue(sCurrentValue))
        {
            /* make sure to set back the valid value from the camera to combobox */
            nIndex = m_EnumComboBox->findText(qstr(sCurrentValue));
            m_EnumComboBox->setCurrentIndex(nIndex);
        }
        else
        {
            m_EnumComboBox->setCurrentIndex(nIndexOld);
            {
                thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (GetEnumValue) " + str(sSelected) + " returned " + str(error) +
                    " " + str(Helper::mapReturnCodeToString(error)));
            }
        }

        #ifdef WIN32
        ::Sleep(5);
        #else
        ::usleep(5000);
        #endif

        /*start Acquisition */
        emit acquisitionStartStop("AcquisitionStart");
        emit updateStatusBar();
        return;
    }

    error = m_FeaturePtr_EnumComboBox->SetValue(sSelected.toUtf8().data());
    if (VmbErrorSuccess != error)
    {
        thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (SetEnumValue) " + str(sSelected) + " returned " + str(error) +
            " " + str(Helper::mapReturnCodeToString(error)));
    }

    if (VmbErrorSuccess == m_FeaturePtr_EnumComboBox->GetValue(sCurrentValue))
    {
        /* make sure to set back the valid value from the camera to combobox */
        nIndex = m_EnumComboBox->findText(QString::fromStdString(sCurrentValue));
        m_EnumComboBox->setCurrentIndex(nIndex);
        emit updateStatusBar();
    }
    else
    {
        m_EnumComboBox->setCurrentIndex(nIndexOld);
        thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (GetEnumValue) " + str(sSelected) + " returned " + str(error) +
            " " + str(Helper::mapReturnCodeToString(error)));
    }
}

/* VmbFeatureDataInt */
void MakoSettingControl::createIntSliderSpinBox(const QModelIndex item)
{
    VmbError_t error;
    QString sFeature_IntSpinBox = getFeatureNameFromModel(item);
    if (!initCurrentFeatureInt(sFeature_IntSpinBox)) {
        return;
    }

    VmbInt64_t nValue = 0;
    if (VmbErrorSuccess == m_FeaturePtr_IntSpinBox->GetValue(nValue))
    {
        if (m_nMaximum < nValue)
            m_FeaturePtr_IntSpinBox->SetValue(m_nMaximum);
    }

    m_IntSpinSliderWidget = new QWidget(this);
    QHBoxLayout* HSpinSliderLayout_Int = new QHBoxLayout(m_IntSpinSliderWidget);
    m_SpinBox_Int = new IntSpinBox(m_IntSpinSliderWidget);
    m_SpinBox_Int->setObjectName("value");      //mark as the element of this dialog that contains the value, used in "updateWidget()"
    QString tn = sFeature_IntSpinBox; //tmp name
    if (tn == "OffsetX" || tn == "OffsetY" || tn == "Height" || tn == "Width" || tn == "Gain" || tn == "ExposureTimeAbs"
        || tn == "AcquisitionFrameRateAbs") {
        m_Slider_Int = new TickSlider(Qt::Horizontal, this);
    }
    else {
        m_Slider_Int = new QSlider(Qt::Horizontal, this);
    }
    m_Slider_Int->installEventFilter(this);
    m_Slider_Int->setTickPosition(QSlider::TicksBelow);

    error = m_FeaturePtr_IntSpinBox->GetIncrement(m_nIncrement);
    if (VmbErrorSuccess == error) {
        m_SpinBox_Int->setSingleStep(m_nIncrement);
        m_Slider_Int->setTickInterval((m_nMaximum - m_nMinimum) / 5); // only show five ticks
        m_Slider_Int->setSingleStep(m_nIncrement);
    }

    m_SpinBox_Int->setRange(m_nMinimum, m_nMaximum);
    m_Slider_Int->setRange(m_nMinimum, m_nMaximum);

    error = m_FeaturePtr_IntSpinBox->GetValue(nValue);
    if (VmbErrorSuccess == error)
    {
        if (nValue > std::numeric_limits<int>::max())
        {
            nValue = std::numeric_limits<int>::max();
        }

        m_SpinBox_Int->setValue(nValue);
        m_Slider_Int->setValue(nValue);
        m_nIntSliderOldValue = nValue;
    }

    m_Slider_Int->setPageStep((m_nMaximum - m_nMinimum) / 5 / m_nIncrement);

    QHBoxLayout* HSpinSliderLayout_Int2 = new QHBoxLayout(m_IntSpinSliderWidget);
    if ((sFeature_IntSpinBox == "OffsetX" || sFeature_IntSpinBox == "OffsetY") && m_nMaximum == 0) {
        // do not add slider, it will cause trouble
    }
    else {
        HSpinSliderLayout_Int2->addWidget(m_Slider_Int);
    }
    HSpinSliderLayout_Int2->addWidget(m_SpinBox_Int);
    HSpinSliderLayout_Int->addLayout(HSpinSliderLayout_Int2);

    QPoint p = QCursor::pos();
    m_IntSpinSliderWidget->setFixedHeight(100);
    m_IntSpinSliderWidget->setMinimumWidth(400);
    m_IntSpinSliderWidget->setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint);
    m_IntSpinSliderWidget->setWindowTitle(m_sCurrentSelectedFeature);
    AdjustOffscreenPosition(p, *m_IntSpinSliderWidget);
    m_IntSpinSliderWidget->move(p.x(), p.y());

    connect(m_Slider_Int, &QSlider::valueChanged, this, &MakoSettingControl::onIntSliderChanged);
    connect(m_SpinBox_Int, &QSpinBox::editingFinished, this, &MakoSettingControl::onIntSpinBoxClick);
    setCursor(Qt::PointingHandCursor);

    m_IntSpinSliderWidget->show();
    
}

void MakoSettingControl::onIntSliderReleased()
{
    onIntSliderChanged(m_Slider_Int->value());
}

void MakoSettingControl::onIntSliderChanged(int nValue)
{
    /* ignore it while pressing mouse and when slider still busy */
    if (m_bIsMousePressed || !m_bIsJobDone)
        return;

    m_bIsJobDone = false;
    int nCurrentValue = nValue;

    if ((m_nIntSliderOldValue > nValue) && (m_Slider_Int->minimum() != nValue))
    {
        int nMod = nValue % m_nIncrement;
        nValue = nValue - nMod;
    }

    if ((0 != (nValue % m_nIncrement)) && (m_Slider_Int->minimum() != nValue) && (m_Slider_Int->maximum() != nValue))
    {
        nValue = nValue + (m_nIncrement - (nValue % m_nIncrement));
    }

    if (m_Slider_Int->hasFocus())
    {
        setIntegerValue(nValue);
    }

    m_nIntSliderOldValue = nCurrentValue;
    m_bIsJobDone = true;
}

void MakoSettingControl::onIntSpinBoxClick()
{
    int nValue = m_SpinBox_Int->value();

    if ((0 != (nValue % m_nIncrement)) && (m_SpinBox_Int->minimum() != nValue) && (m_SpinBox_Int->maximum() != nValue))
    {
        nValue = (nValue + (m_nIncrement - (nValue % m_nIncrement)));
    }
    try {
        setIntegerValue(nValue);

    }
    catch (ChimeraError& e) {
        errBox(e.trace());
    }
}


bool MakoSettingControl::initCurrentFeatureInt(const QString& name)
{
    if (!isFeatureWritable(name)) {
        return false;
    }
    m_sCurrentSelectedFeature = m_sFeature_IntSpinBox = name;
    m_FeaturePtr_IntSpinBox = getFeaturePtrFromMap(m_sFeature_IntSpinBox);
    if (FeaturePtr() == m_FeaturePtr_IntSpinBox)
        return false;
    resetControl();
    VmbError_t error = m_FeaturePtr_IntSpinBox->GetRange(m_nMinimum, m_nMaximum);
    if (VmbErrorSuccess == error)
    {
        if (m_nMinimum < std::numeric_limits<int>::min())
        {
            m_nMinimum = std::numeric_limits<int>::min();
        }

        if (m_nMaximum > std::numeric_limits<int>::max())
        {
            m_nMaximum = std::numeric_limits<int>::max();
        }
        m_FeaturePtr_IntSpinBox->GetIncrement(m_nIncrement);
        m_nMaximum = m_nMaximum - ((m_nMaximum - m_nMinimum) % m_nIncrement);
        return true;
    }
    else {
        thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (GetRange) returned " + str(error) +
            " " + str(Helper::mapReturnCodeToString(error)));
        return false;
    }
    /* use a line edit to show reg add in Hex, no :(  */
    std::string sRepresentation;
    m_FeaturePtr_IntSpinBox->GetRepresentation(sRepresentation);
    QString sName = getFeatureNameFromMap(m_sCurrentSelectedFeature);
    if (VmbErrorSuccess == error &&
        (sRepresentation == "HexNumber" || Helper::needsIPv4Format(sName, sRepresentation) || m_nMaximum > 9999))
    {
        // not doing hex manipulation in Chimera, always use VimbaViewer if want to change the ip related stuff
        return false;
    }
}

void MakoSettingControl::setIntegerValue(const int& nValue)
{
    VmbError_t error;

    if (0 == m_sFeature_IntSpinBox.compare("Width") || 0 == m_sFeature_IntSpinBox.compare("Height"))
    {
        /* stop Acquisition */
        emit acquisitionStartStop("AcquisitionStop");

        error = m_FeaturePtr_IntSpinBox->SetValue(nValue);
        if (VmbErrorSuccess != error)
        {
            thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (SetValue) " + str(nValue) + " returned " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
        }

        updateCurrentIntValue();

        /*start Acquisition */
        emit acquisitionStartStop("AcquisitionStart");
        return;
    }

    error = m_FeaturePtr_IntSpinBox->SetValue(nValue);
    if (VmbErrorSuccess != error)
    {
        thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (SetValue) " + str(nValue) + " returned " + str(error) +
            " " + str(Helper::mapReturnCodeToString(error)));
    }

    updateCurrentIntValue();
    emit updateStatusBar();
}

void MakoSettingControl::updateCurrentIntValue()
{
    if (m_IntSpinSliderWidget == nullptr) {
        return;
    }
    VmbInt64_t nCurrentValue = 0;
    VmbError_t error = m_FeaturePtr_IntSpinBox->GetValue(nCurrentValue);
    if (VmbErrorSuccess == error)
    {
        /* make sure to set back the valid value from the camera to slider and spinbox */
        m_SpinBox_Int->setValue(nCurrentValue);
        m_Slider_Int->setValue(nCurrentValue);
    }
    else
    {
        thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (GetValue) " + str(nCurrentValue) + " returned " + str(error) +
            " " + str(Helper::mapReturnCodeToString(error)));
    }
}

/* VmbFeatureDataFloat */
void MakoSettingControl::createFloatSliderEditBox(const QModelIndex item)
{
    VmbError_t error;
    QString sFeature_FloatSliderEditBox = getFeatureNameFromModel(item);
    if (!initCurrentFeatureFloat(sFeature_FloatSliderEditBox)) {
        return;
    }

    std::string sRepresentation;
    m_FeaturePtr_FloatSliderSpinBox->GetRepresentation(sRepresentation);
    /* use a logarithmic slider for exposure , no, never :( */

    m_FloatSliderEditWidget = new QWidget(this);
    QHBoxLayout* HSpinSliderLayout_Float = new QHBoxLayout(m_FloatSliderEditWidget);

    /* Line Edit */
    m_EditBox_Float = new QLineEdit(m_FloatSliderEditWidget);
    m_EditBox_Float->setObjectName("value");    //mark as the element of this dialog that contains the value, used in "updateWidget()"

    /* GigE: Gain, Hue
    *  1394: Trigger Delay
    *  They will be treated as Floating in fact they are int
    */
    double dValue = 0;
    error = m_FeaturePtr_FloatSliderSpinBox->GetValue(dValue);

    if (VmbErrorSuccess == error)
    {
        m_EditBox_Float->setText(qstr(dValue, 3));
    }
    else
    {
        thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (GetValue) " + str(QString::number(dValue, 'g', 16)) + " returned " + str(error) +
            " " + str(Helper::mapReturnCodeToString(error)));
    }

    /* Slider */
    m_Slider_Float = new DoubleTickSlider(Qt::Horizontal, m_FloatSliderEditWidget);
    m_Slider_Float->setFocusPolicy(Qt::StrongFocus);
    m_Slider_Float->installEventFilter(this);
    m_Slider_Float->setTotalSteps(20000);
    m_Slider_Float->setDoubleScale(m_dMinimum, m_dMaximum);
    m_Slider_Float->setDoubleTickInterval((m_dMaximum - m_dMinimum) / 5); // only show five ticks
    m_Slider_Float->setTickPosition(QSlider::TicksBelow);
    if (abs(m_dIncrement) > 0.0000001)
    {
        m_Slider_Float->setDoubleSingleStep(m_dIncrement);
    }
    if (sFeature_FloatSliderEditBox=="Gain") // gain is treated as float but indeed it is an int
    {
        m_Slider_Float->setDoubleSingleStep(1.0);
    }

    if (0 == m_sFeature_FloatSliderSpinBox.compare("ExposureTimeAbs")) {
        m_Slider_Float->setDoubleScale(m_dMinimum, m_dMaximum / 100);
        m_Slider_Float->setDoubleTickInterval((m_dMaximum / 100 - m_dMinimum) / 5); // only show five ticks
        if (abs(m_dIncrement) > 0.0000001) {
            m_Slider_Float->setDoubleSingleStep(m_dIncrement);
        }
        else {
            m_Slider_Float->setDoubleSingleStep(100.0/*us*/);
        }
    }
        
    m_Slider_Float->setDoubleValue(dValue);


    QHBoxLayout* HSliderEditLayout_Float2 = new QHBoxLayout(m_FloatSliderEditWidget);
    HSliderEditLayout_Float2->addWidget(m_Slider_Float);
    m_Slider_Float->setMinimumWidth(200);
    m_EditBox_Float->setMinimumWidth(60);
    m_EditBox_Float->setMaximumWidth(80);
    HSliderEditLayout_Float2->addWidget(m_EditBox_Float);
    m_EditBox_Float->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    HSpinSliderLayout_Float->addLayout(HSliderEditLayout_Float2);

    QPoint p = QCursor::pos();
    m_FloatSliderEditWidget->setFixedHeight(100);
    m_FloatSliderEditWidget->setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint);
    m_FloatSliderEditWidget->setWindowTitle(m_sCurrentSelectedFeature);
    AdjustOffscreenPosition(p, *m_FloatSliderEditWidget);
    m_FloatSliderEditWidget->move(p.x(), p.y());
    m_FloatSliderEditWidget->show();
    m_FloatSliderEditWidget->setFocus();

    /* As of Vimba >= 1.4, float features other than ExposureTime don't use spin box anymore */
    connect(m_Slider_Float, &DoubleTickSlider::doubleValueChanged, this, &MakoSettingControl::onFloatSliderChanged);
    connect(m_EditBox_Float, &QLineEdit::editingFinished, this, &MakoSettingControl::onFloatEditFinished);
    setCursor(Qt::PointingHandCursor);
    m_FloatSliderEditWidget->show();
    
}

void MakoSettingControl::onFloatSliderReleased()
{
    onFloatSliderChanged(m_Slider_Float->doubleValue());
}

void MakoSettingControl::onFloatSliderChanged(double dValue)
{
    /* ignore it while pressing mouse and when slider still busy */
    if (m_bIsMousePressed || !m_bIsJobDone)
        return;

    m_bIsJobDone = false;

    if (dValue > (m_dMaximum * 0.995))
        dValue = m_dMaximum;

    if (m_Slider_Float->hasFocus())
    {
        m_EditBox_Float->setText(qstr(dValue, 3));
        /* As of Vimba >= 1.4, float features other than ExposureTime don't use spin box anymore */
        onFloatEditFinished();
    }

    m_bIsJobDone = true;
}

void MakoSettingControl::onFloatEditFinished()
{
    double dValue = m_EditBox_Float->text().toDouble() * 1.0000000000001; // allow some float imprecision
    setFloatingValue(dValue);
}

bool MakoSettingControl::initCurrentFeatureFloat(const QString& name)
{
    if (!isFeatureWritable(name)) {
        return false;
    }
    m_sCurrentSelectedFeature = m_sFeature_FloatSliderSpinBox = name;
    m_FeaturePtr_FloatSliderSpinBox = getFeaturePtrFromMap(name);
    if (FeaturePtr() == m_FeaturePtr_FloatSliderSpinBox)
        return false;

    resetControl();
    VmbError_t error = m_FeaturePtr_FloatSliderSpinBox->GetRange(m_dMinimum, m_dMaximum);
    if (VmbErrorSuccess == error)
    {
        /* if there's an increment, compute the correct maximum (allowing some float uncertainty) */
        if (VmbErrorSuccess == m_FeaturePtr_FloatSliderSpinBox->GetIncrement(m_dIncrement))
        {
            double dSteps = floor((m_dMaximum - m_dMinimum) / m_dIncrement * 1.000000000001);
            double dMaximum = m_dMinimum + dSteps * m_dIncrement * 1.000000000001;
            if (dMaximum < m_dMaximum)
                m_dMaximum = dMaximum;
        }
        else
        {
            m_dIncrement = 0.0;
        }
        return true;
    }
    else
    {
        thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (GetRange) " + " returned " + str(error) +
            " " + str(Helper::mapReturnCodeToString(error)));
        return false;
    }
}

void MakoSettingControl::setFloatingValue(const double& dValue)
{
    double dCurrentValue = dValue;
    VmbError_t error;

    /* restrict to the allowed values */
    if (m_dMaximum < dValue)
    {
        dCurrentValue = m_dMaximum;
    }
    else if (m_dMinimum > dValue)
    {
        dCurrentValue = m_dMinimum;
    }
    // removed handling for increment because of rounding errors
    // else if ( m_dIncrement != 0.0 )
    // {
    //     dCurrentValue = floor((dValue-m_dMinimum)/m_dIncrement)*m_dIncrement + m_dMinimum;
    // }
    error = m_FeaturePtr_FloatSliderSpinBox->SetValue(dCurrentValue);

    if (VmbErrorSuccess == error)
    {
        updateCurrentFloatValue();
        /* reset all fps counters whenever shutter value has been changed */
        if (0 == m_sFeature_FloatSliderSpinBox.compare("Exposure Time") ||
            0 == m_sFeature_FloatSliderSpinBox.compare("ExposureTimeAbs"))
        {
            emit resetFPS();
        }
        emit updateStatusBar();
    }
    else
    {
        thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (SetValue) " + str(dValue) + " returned " + str(error) +
            " " + str(Helper::mapReturnCodeToString(error)));
    }
}

void MakoSettingControl::updateCurrentFloatValue()
{
    if (m_FloatSliderEditWidget == nullptr) {
        return;
    }
    double dCurrentValue;
    VmbError_t error = m_FeaturePtr_FloatSliderSpinBox->GetValue(dCurrentValue);
    if (VmbErrorSuccess == error)
    {
        const QString newValue = QString::number(dCurrentValue);
        if (m_EditBox_Float->text() != newValue)
        {
            m_EditBox_Float->setText(qstr(newValue, 3));
        }

        if (!m_Slider_Float->hasFocus())
        { /* to handle Gamma feature. Make sure to set the current feature value back to slider */
            m_Slider_Float->setValue(dCurrentValue);
        }
    }
}

/* VmbFeatureDataBool */
void MakoSettingControl::createBooleanCheckBox(const QModelIndex item)
{
    QString sFeature_CheckBox = getFeatureNameFromModel(item);

    if (isFeatureWritable(sFeature_CheckBox))
    {
        m_sCurrentSelectedFeature = m_sFeature_CheckBox = sFeature_CheckBox;

        m_FeaturePtr_CheckBox = getFeaturePtrFromMap(m_sFeature_CheckBox);
        if (FeaturePtr() == m_FeaturePtr_CheckBox)
            return;

        resetControl();

        m_BooleanWidget = new QWidget(this);
        QHBoxLayout* HBooleanLayout = new QHBoxLayout(m_BooleanWidget);
        m_CheckBox_Bool = new QCheckBox(QString(""), m_BooleanWidget);
        m_CheckBox_Bool->setMaximumHeight(40);
        m_CheckBox_Bool->setObjectName("value");    //mark as the element of this dialog that contains the value, used in "updateWidget()"
        QHBoxLayout* HBooleanLayout2 = new QHBoxLayout(m_BooleanWidget);
        HBooleanLayout2->addWidget(m_CheckBox_Bool);
        HBooleanLayout->addLayout(HBooleanLayout2);

        bool bValue = false;
        VmbError_t error = m_FeaturePtr_CheckBox->GetValue(bValue);
        if (VmbErrorSuccess == error)
        {
            m_CheckBox_Bool->setChecked(bValue);
        }
        else
        {
            m_CheckBox_Bool->setChecked(false);
            thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (GetValue) " + " returned " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
        }

        QPoint p = QCursor::pos();
        m_BooleanWidget->setFixedHeight(60);
        m_CheckBox_Bool->setText(m_sCurrentSelectedFeature);

        m_BooleanWidget->setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint);
        m_BooleanWidget->setWindowTitle(m_sCurrentSelectedFeature);
        AdjustOffscreenPosition(p, *m_BooleanWidget);
        m_BooleanWidget->move(p.x(), p.y());
        
        connect(m_CheckBox_Bool, &QCheckBox::clicked, this, &MakoSettingControl::onBoolCheckBoxClick);
        setCursor(Qt::PointingHandCursor);
        m_BooleanWidget->show();
    }
}

void MakoSettingControl::onBoolCheckBoxClick(bool bValue)
{
    bool bCurrentValue = false;

    VmbError_t error = m_FeaturePtr_CheckBox->SetValue(bValue);
    if (VmbErrorSuccess == error)
    {
        error = m_FeaturePtr_CheckBox->GetValue(bCurrentValue);
        if (VmbErrorSuccess == error)
        {
            QString sValue = "false";
            (true == bCurrentValue) ? sValue = "true" : sValue = "false";
            m_CheckBox_Bool->setChecked(bCurrentValue);
            emit updateStatusBar();
        }
        else
        {
            thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (GetValue) " + " returned " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
        }
    }
    else
    {
        thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (SetValue) " + str(bValue) + " returned " + str(error) +
            " " + str(Helper::mapReturnCodeToString(error)));
    }
}

/* VmbFeatureDataString */
void MakoSettingControl::createStringEditBox(const QModelIndex item)
{
    QString sFeature_StringEditBox = getFeatureNameFromModel(item);

    if (isFeatureWritable(sFeature_StringEditBox))
    {
        m_sCurrentSelectedFeature = m_sFeature_StringEditBox = sFeature_StringEditBox;

        m_FeaturePtr_StringEditBox = getFeaturePtrFromMap(m_sFeature_StringEditBox);
        if (FeaturePtr() == m_FeaturePtr_StringEditBox)
            return;

        resetControl();

        m_EditWidget = new QWidget(this);
        QHBoxLayout* HEditLayout = new QHBoxLayout(m_EditWidget);
        m_TextEdit_String = new QLineEdit(m_EditWidget);
        m_TextEdit_String->setObjectName("value");  //mark as the element of this dialog that contains the value, used in "updateWidget()"
        m_TextEdit_String->setMaximumHeight(20);
        QHBoxLayout* HEditLayout2 = new QHBoxLayout(m_EditWidget);
        HEditLayout2->addWidget(m_TextEdit_String);
        HEditLayout->addLayout(HEditLayout2);

        std::string sValue;
        VmbError_t error = m_FeaturePtr_StringEditBox->GetValue(sValue);

        if (VmbErrorSuccess == error)
            m_TextEdit_String->setText(qstr(sValue));
        else
            m_TextEdit_String->setText(qstr(" "));

        QPoint p = QCursor::pos();
        m_EditWidget->setFixedHeight(60);
        m_EditWidget->setWindowFlags(Qt::Tool | Qt::WindowCloseButtonHint);
        m_EditWidget->setWindowTitle(m_sCurrentSelectedFeature);
        AdjustOffscreenPosition(p, *m_EditWidget);
        m_EditWidget->move(p.x(), p.y());

        connect(m_TextEdit_String, &QLineEdit::returnPressed, this, &MakoSettingControl::onEditText);
        setCursor(Qt::PointingHandCursor);
        m_EditWidget->show();
    }
}

void MakoSettingControl::onEditText()
{
    QString sText = m_TextEdit_String->text();
    VmbError_t error = m_FeaturePtr_StringEditBox->SetValue((const char*)sText.toUtf8());

    if (VmbErrorSuccess == error)
    {
        std::string sCurrentValue;
        error = m_FeaturePtr_StringEditBox->GetValue(sCurrentValue);
        if (VmbErrorSuccess == error)
        {
            m_TextEdit_String->setText(qstr(sCurrentValue));
            emit updateStatusBar();
        }
        else
        {
            m_TextEdit_String->setText("");
            thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (GetStringValue) " + " returned " + str(error) +
                " " + str(Helper::mapReturnCodeToString(error)));
        }
    }
    else
    {
        thrower("ERROR Feature: " + str(m_sCurrentSelectedFeature) + " (SetStringValue) " + str(sText) + " returned " + str(error) +
            " " + str(Helper::mapReturnCodeToString(error)));
    }
}

bool MakoSettingControl::eventFilter(QObject* object, QEvent* event)
{
    /* set the slider focus back when loosing on scrolling, e.g scrolling right after changing spinbox */
    if (event->type() == QEvent::Wheel)
    {
        if (object == m_Slider_Int)
        {
            if (!m_Slider_Int->hasFocus())
                m_Slider_Int->setFocus();
        }

        if (object == m_Slider_Float)
        {
            if (!m_Slider_Float->hasFocus())
                m_Slider_Float->setFocus();
        }
    }

    if (event->type() == QEvent::MouseButtonPress)
    {
        m_bIsMousePressed = true;
    }

    if (event->type() == QEvent::MouseButtonRelease)
    {
        m_bIsMousePressed = false;

        if (object == m_Slider_Int)
            onIntSliderReleased();

        if (object == m_Slider_Float)
            onFloatSliderReleased();
    }

    return false;
}

void MakoSettingControl::resetControl()
{
    /* delete button */
    if (nullptr != m_ButtonWidget)
    {
        disconnect(m_CmdButton, &QPushButton::clicked, this, &MakoSettingControl::onCmdButtonClick);
        delete m_ButtonWidget;

        m_ButtonWidget = nullptr;
        m_CmdButton = nullptr;

    }

    /* delete ComboBox */
    if (nullptr != m_EnumComboBox)
    {
        disconnect(m_EnumComboBox, qOverload<const QString&>(&QComboBox::activated), this, &MakoSettingControl::onEnumComboBoxClick);
        delete m_ComboWidget;

        m_ComboWidget = nullptr;
        m_EnumComboBox = nullptr;

    }

    /* delete SpinBox+Slider Int */
    if ((nullptr != m_SpinBox_Int) && (nullptr != m_Slider_Int) && (nullptr != m_IntSpinSliderWidget))
    {
        removeEventFilter(m_Slider_Int);
        disconnect(m_Slider_Int, &QSlider::valueChanged, this, &MakoSettingControl::onIntSliderChanged);
        disconnect(m_SpinBox_Int, &QSpinBox::editingFinished, this, &MakoSettingControl::onIntSpinBoxClick);
        delete m_IntSpinSliderWidget;

        m_IntSpinSliderWidget = nullptr;
        m_SpinBox_Int = nullptr;
        m_Slider_Int = nullptr;
    }

    /* delete SpinBox+Slider Float */
    if ((nullptr != m_EditBox_Float) && (nullptr != m_Slider_Float) && (nullptr != m_FloatSliderEditWidget))
    {
        removeEventFilter(m_Slider_Float);
        disconnect(m_Slider_Float, &DoubleTickSlider::doubleValueChanged, this, &MakoSettingControl::onFloatSliderChanged);
        disconnect(m_EditBox_Float, &QLineEdit::editingFinished, this, &MakoSettingControl::onFloatEditFinished);
        delete m_FloatSliderEditWidget;

        m_FloatSliderEditWidget = nullptr;
        m_EditBox_Float = nullptr;
        m_Slider_Float = nullptr;      
    }

    /* delete bool CheckBox  */
    if (nullptr != m_CheckBox_Bool)
    {
        disconnect(m_CheckBox_Bool, &QCheckBox::clicked, this, &MakoSettingControl::onBoolCheckBoxClick);
        delete m_BooleanWidget;

        m_BooleanWidget = nullptr;
        m_CheckBox_Bool = nullptr;
    }

    /* delete TextEdit */
    if (nullptr != m_TextEdit_String)
    {
        disconnect(m_TextEdit_String, &QLineEdit::returnPressed, this, &MakoSettingControl::onEditText);
        delete m_EditWidget;

        m_EditWidget = nullptr;
        m_TextEdit_String = nullptr;
    }
}

void MakoSettingControl::AdjustOffscreenPosition(QPoint& p, QWidget& parentWidget)
{
    Q_ASSERT(&parentWidget && "parentWidget is NULL reference");

    /* out of screen? first identify the current screen */
    QDesktopWidget desktop;
    const QRect currentScreenGeometry = desktop.screenGeometry(desktop.screenNumber(p));

    parentWidget.adjustSize();

    /* target position: above and to the right of the targeted position p.x/p.y. */
    p.setY(p.y() - 10 - parentWidget.frameGeometry().height());


    /* if parts of the window would appear offscreen, move to the left */
    int horizontalCorrection = (p.x() - currentScreenGeometry.right()) + parentWidget.frameGeometry().width();
    if (horizontalCorrection > 0)
        p.setX(p.x() - horizontalCorrection);

    /*  if vertical position would be offscreen, show below the current line */
    if (currentScreenGeometry.top() - p.y() > 0)
        p.setY(p.y() + 24 + parentWidget.frameGeometry().height());
}

template <typename COMPOUND_WIDGET>
void MakoSettingControl::updateEditWidget(COMPOUND_WIDGET* w, const QVariant& value, bool bIsWritable)
{
    w->setEnabled(bIsWritable);
    if (bIsWritable)
    {
        QLineEdit* pChild = w-> template findChild<QLineEdit*>(QString("value"));
        if (nullptr != pChild
            && !pChild->isModified()
            )
        {
            const QString newValue = value.toString();
            if (pChild->text() != newValue)
            {
                pChild->setText(newValue);
            }
        }
    }
}

void MakoSettingControl::updateWidget(const bool bIsWritable, const QVariant& value)
{
    if (nullptr != m_IntSpinSliderWidget)
    {
        m_IntSpinSliderWidget->setEnabled(bIsWritable);
        if (bIsWritable)
        {
            IntSpinBox* spinBox = m_IntSpinSliderWidget->findChild<IntSpinBox*>(QString("value"));
            if (nullptr != spinBox)
            {
                int newValue = value.toInt();
                if (spinBox->value() != newValue
                    && !spinBox->hasFocus())
                {
                    spinBox->setValue(newValue);
                }
            }
        }
    }

    else if (nullptr != m_ButtonWidget)
        m_ButtonWidget->setEnabled(bIsWritable);

    else if (nullptr != m_ComboWidget)
        m_ComboWidget->setEnabled(bIsWritable);

    else if (nullptr != m_FloatSliderEditWidget)
    {
        if (bIsWritable)
        {
            QLineEdit* lineEdit = m_FloatSliderEditWidget->findChild<QLineEdit*>(QString("value"));
            if (nullptr != lineEdit)
            {
                QString newValue = qstr(value.toFloat(), 3);
                if (lineEdit->text() != newValue
                    && !lineEdit->hasFocus())
                {
                    lineEdit->setText(newValue);
                }
            }
        }
    }

    else if (nullptr != m_BooleanWidget)
    {
        m_BooleanWidget->setEnabled(bIsWritable);
        if (bIsWritable)
            m_BooleanWidget->findChild<QCheckBox*>(QString("value"))->setChecked(value.toBool());
    }

    else if (nullptr != m_EditWidget)
    {
        updateEditWidget(m_EditWidget, value, bIsWritable);
    }

}

template<typename dataType>
dataType MakoSettingControl::getFeatureValue(std::string feaName, std::string& errstr)
{
    if (m_pCam == nullptr) {
        return dataType(); //means in SAFEMODE
    }
    FeaturePtr feature = getFeaturePtrFromMap(qstr(feaName));
    VmbErrorType error;
    dataType val;
    error = feature->GetValue(val);
    if (VmbErrorSuccess != error) {
        errstr += "Error in getting " + feaName + ", with error" + str(error) + ", " + str(Helper::mapReturnCodeToString(error)) + "\n";
    }
    return val;
}

template<typename dataType>
void MakoSettingControl::setFeatureValue(std::string feaName, dataType val, std::string& errstr)
{
    if (m_pCam == nullptr) {
        return; //means in SAFEMODE
    }
    FeaturePtr feature = getFeaturePtrFromMap(qstr(feaName));
    VmbErrorType error;
    error = feature->SetValue(val);
    if (VmbErrorSuccess != error) {
        errstr += "Error in setting " + feaName + ", with error" + str(error) + ", " + str(Helper::mapReturnCodeToString(error)) + "\n";
    }
}

void MakoSettingControl::setSettings(MakoSettings newSettings)
{
    updateCurrentSettings();
    MakoSettings originalSetting = currentSettings;
    currentSettings = newSettings;
    
    std::string errstr("");
    //emit acquisitionStartStop("AcquisitionStop");
    if (currentSettings.rawGain != originalSetting.rawGain) {
        setFeatureValue<double>("Gain", currentSettings.rawGain, errstr);
    }
    if (!(currentSettings.trigOn ^ originalSetting.trigOn)) {
        setFeatureValue("TriggerMode", currentSettings.trigOn ? "On" : "Off", errstr);
    }
    if (currentSettings.triggerMode != originalSetting.triggerMode) {
        setFeatureValue("TriggerSource", MakoTrigger::toStr(currentSettings.triggerMode).c_str(), errstr);
    }
    if (currentSettings.exposureTime != originalSetting.exposureTime) {
        setFeatureValue<double>("ExposureTimeAbs", currentSettings.exposureTime, errstr);
    }
    if (abs(currentSettings.frameRate - originalSetting.frameRate) < DBL_EPSILON * 10) {
        setFeatureValue<double>("AcquisitionFrameRateAbs", currentSettings.frameRate, errstr);
    }
    if (currentSettings.dims.left != originalSetting.dims.left) {
        setFeatureValue<VmbInt64_t>("OffsetX", currentSettings.dims.left, errstr);
    }
    if (currentSettings.dims.width() != originalSetting.dims.width()) {
        setFeatureValue<VmbInt64_t>("Width", currentSettings.dims.width(), errstr);
    }
    if (currentSettings.dims.bottom != originalSetting.dims.bottom) {
        setFeatureValue<VmbInt64_t>("OffsetY", currentSettings.dims.bottom, errstr);
    }
    if (currentSettings.dims.height() != originalSetting.dims.height()) {
        setFeatureValue<VmbInt64_t>("Height", currentSettings.dims.height(), errstr);
    }

    //emit acquisitionStartStop("AcquisitionStart");
    //Sleep(500);
    //emit acquisitionStartStop("AcquisitionStop");
    //updateRegisterFeature();
    //updateCurrentSettings();
    if (!errstr.empty()) {
        thrower("Error in setting MakoSetting" + errstr);
    }
}

void MakoSettingControl::updateCurrentSettings()
{
    FeaturePtr feature;
    double dval = 0;
    VmbInt64_t ival = 0;
    bool bval = 0;
    std::string sval;
    std::string errorStr("");
    
    currentSettings.rawGain = getFeatureValue<double>("Gain", errorStr);
    sval = getFeatureValue<std::string>("TriggerMode", errorStr);
    currentSettings.trigOn = (sval=="On") ? true : false;

    sval = getFeatureValue<std::string>("TriggerSource", errorStr);
    currentSettings.triggerMode = MakoTrigger::fromStr(sval);

    currentSettings.exposureTime = getFeatureValue<double>("ExposureTimeAbs", errorStr);
    currentSettings.frameRate = getFeatureValue<double>("AcquisitionFrameRateAbs", errorStr);

    currentSettings.dims.left = getFeatureValue<VmbInt64_t>("OffsetX", errorStr);
    currentSettings.dims.right = currentSettings.dims.left + getFeatureValue<VmbInt64_t>("Width", errorStr) - 1;
    currentSettings.dims.bottom = getFeatureValue<VmbInt64_t>("OffsetY", errorStr);
    currentSettings.dims.top = currentSettings.dims.bottom + getFeatureValue<VmbInt64_t>("Height", errorStr) - 1;
    currentSettings.dims.horizontalBinning = getFeatureValue<VmbInt64_t>("BinningHorizontal", errorStr);
    currentSettings.dims.verticalBinning = getFeatureValue<VmbInt64_t>("BinningVertical", errorStr);
    if (!errorStr.empty()) {
        thrower("Error in update mako Current Settings" + errorStr);
    }
}

void MakoSettingControl::setExpActive(bool active)
{
    currentSettings.expActive = active;
}

void MakoSettingControl::setPicsPerRep(int picsperrep)
{
    currentSettings.picsPerRep = picsperrep;
}

std::pair<VmbInt64_t, VmbInt64_t> MakoSettingControl::getMaxImageSize()
{
    std::string errorStr("");
    VmbInt64_t hm = getFeatureValue<VmbInt64_t>("HeightMax", errorStr);
    VmbInt64_t wm = getFeatureValue<VmbInt64_t>("WidthMax", errorStr);
    return std::pair<VmbInt64_t, VmbInt64_t>(hm,wm);
}

void MakoSettingControl::closeControls()
{
    resetControl();
}
