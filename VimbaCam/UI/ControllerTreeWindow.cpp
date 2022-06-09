//Description: All about features control tree

/* define this to use std::numeric_limits */
#define NOMINMAX

#include <limits>
#include <iostream>

#include <QCheckBox>
#include <QItemDelegate>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QStandardItemModel>

#include "../ExternLib/qwt/qwt_slider.h"
#include "../ExternLib/qwt/qwt_scale_engine.h"

#include "ControllerTreeWindow.h"
#include "ExComboBox.h"
#include "Helper.h"
#include "HexEditor/HexMainWindow.h"
#include "IntSpinBox.h"
#include "MultiCompleter.h"
#include "SortFilterProxyModel.h"
//#include "SplashScreen.h"

#include <FeatureObserver.h>

using AVT::VmbAPI::StringVector;

#ifdef WIN32
    #include <windows.h>
#else
    #include <unistd.h>
    #include <cstring>
#endif

/* controlling row/height size in tree */
class ItemDelegate: public QItemDelegate
{
public:
    ItemDelegate() {}
    QSize sizeHint( const QStyleOptionViewItem& option, const QModelIndex& index ) const
    {
        QSize s = QItemDelegate::sizeHint( option, index );
        s.setHeight( row_height );
        return s;
    }

    void setRowHeight( const unsigned char height )
    {
        row_height = height;
    }

protected:
    unsigned char row_height;
};

ControllerTreeWindow::ControllerTreeWindow ( QString sID, QWidget *parent, bool bAutoAdjustPacketSize, CameraPtr pCam )
    : QTreeView ( parent ), m_TreeDelegate ( NULL ),
    m_HBooleanLayout          ( NULL ), m_HBooleanLayout2          ( NULL ), m_BooleanWidget         ( NULL ), m_CheckBox_Bool   ( NULL ),
    m_HEditLayout             ( NULL ), m_HEditLayout2             ( NULL ), m_EditWidget            ( NULL ), m_TextEdit_String ( NULL ),
    m_HButtonLayout           ( NULL ), m_HButtonLayout2           ( NULL ), m_ButtonWidget          ( NULL ), m_CmdButton       ( NULL ),
    m_HComboLayout            ( NULL ), m_HComboLayout2            ( NULL ), m_ComboWidget           ( NULL ), m_EnumComboBox    ( NULL ),
    m_HLineEditLayout         ( NULL ), m_HLineEditLayout2         ( NULL ), m_LineEditWidget        ( NULL ), m_LineEdit        ( NULL ), m_HexLabel ( NULL ),
    m_HSpinSliderLayout_Int   ( NULL ), m_HSpinSliderLayout_Int2   ( NULL ), m_IntSpinSliderWidget   ( NULL ), m_SpinBox_Int     ( NULL ), m_Slider_Int   ( NULL ),
    m_HSpinSliderLayout_Float ( NULL ), m_HSliderEditLayout_Float2 ( NULL ), m_FloatSliderEditWidget ( NULL ), m_EditBox_Float   ( NULL ), m_Slider_Float ( NULL ),
    m_HexWindow               ( NULL ), m_LogSliderWidget          ( NULL ), m_FeaturesPollingTimer  ( NULL ),
    m_nSliderStep ( 0 ), m_dMinimum ( 0 ), m_dMaximum ( 0 ), m_dIncrement ( 0 ), m_nIntSliderOldValue ( 0 ), m_bIsGigE ( false ), m_bIsTimeout ( false ),
    m_pFeatureObs( new FeatureObserver(pCam)), m_ListTreeError ( VmbErrorOther ), m_bIsTooltipOn ( true ), m_bIsJobDone ( true ), m_bIsMousePressed ( false ), m_bIsBusy ( false )
{
    m_pCam = pCam;
    m_sCameraID = sID;
    setupTree();
    connect((FeatureObserver*)(SP_ACCESS(m_pFeatureObs)), SIGNAL(setEventMessage(const QStringList &)), this, SLOT(onSetEventMsg(const QStringList &)));
    this->setStyleSheet(QString::fromUtf8("QToolTip {""}"));

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

    QStringList CompletionList;

    VmbError_t error = m_pCam->GetFeatures(m_featPtrVec);
    if(VmbErrorSuccess == error)
    {
        for( unsigned int i=0; i < m_featPtrVec.size(); i++ )
        {
            VmbFeatureVisibilityType visibilityType;
            m_featPtrVec.at(i)->GetVisibility(visibilityType);

            std::string sDisplayName;
            m_featPtrVec.at(i)->GetDisplayName(sDisplayName);

            /* Save DisplayName for filter completer */
            CompletionList << QString::fromStdString(sDisplayName);

            if((VmbFeatureVisibilityBeginner == visibilityType) || (VmbFeatureVisibilityExpert == visibilityType) || (VmbFeatureVisibilityGuru == visibilityType))
            {
                sortCategoryAndAttribute(m_featPtrVec.at(i), m_ModelGuru);
                mapInformation(QString::fromStdString(sDisplayName), m_featPtrVec.at(i));
                m_featPtrMap[QString::fromStdString(sDisplayName)] = m_featPtrVec.at(i);
                std::string sFeatureName;
                m_featPtrVec.at(i)->GetName(sFeatureName);
                m_DisplayFeatureNameMap[QString::fromStdString(sDisplayName)] = QString::fromStdString(sFeatureName);
            }
        }
    }

    /* Features string completer for filter */
    m_StringCompleter = new MultiCompleter(CompletionList, this);
    m_StringCompleter->setCaseSensitivity(Qt::CaseInsensitive);

    m_ListTreeError = error;
    m_Model = m_ModelGuru;

    m_ProxyModel = new SortFilterProxyModel(this);
    m_ProxyModel->setDynamicSortFilter(true);
    setModel(m_ProxyModel);
    m_ProxyModel->setSourceModel(m_Model);

    resizeColumnToContents(0);
    /* used to set the maximum height of the row in tree */
    m_TreeDelegate = new ItemDelegate();
    m_TreeDelegate->setRowHeight(20);
    setItemDelegate(m_TreeDelegate);

    connect( this, SIGNAL( expanded(const QModelIndex &) ), this, SLOT( expand(const QModelIndex &) ) );
    connect( this, SIGNAL( expanded(const QModelIndex &) ), this, SLOT( updateColWidth()) );
    connect( this, SIGNAL( collapsed(const QModelIndex &) ), this, SLOT( collapse(const QModelIndex &) ) );
    connect((FeatureObserver*)(SP_ACCESS(m_pFeatureObs)), SIGNAL(setChangedFeature(const QString &, const QString &, const bool &)),
            this, SLOT(onSetChangedFeature(const QString &, const QString &, const bool &)));
    connect( this, SIGNAL( clicked(const QModelIndex &) ), this, SLOT( onClicked(const QModelIndex &) ) );

    sortByColumn(0, Qt::AscendingOrder);
    setSelectionMode(QAbstractItemView::SingleSelection);

    /* Auto Adjust Packet Size */
    m_bAutoAdjustPacketSize = bAutoAdjustPacketSize;
    if(m_bIsGigE && m_bAutoAdjustPacketSize)
    {
        FeaturePtr FeatPtr = getFeaturePtr("GVSP Adjust Packet Size");
        if( NULL == FeatPtr)
            return;

        error = FeatPtr->RunCommand();
        if(VmbErrorSuccess == error)
        {
            QTimer::singleShot(7000, this, SLOT(setAdjustPacketSizeTimeout()));

            bool bIsDone = false;
            while((!bIsDone) && (VmbErrorSuccess == error))
            {
                QCoreApplication::processEvents();
                if(m_bIsTimeout)
                {
                    m_ListTreeError = 7000; //7000: "Timeout"
                    return;
                }
                error = FeatPtr->IsCommandDone(bIsDone);
            }

            m_ListTreeError = error;
        }
    }

    m_FeaturesPollingTimer = new QTimer ( this );
    connect (m_FeaturesPollingTimer, SIGNAL(timeout()), this, SLOT(pollingFeaturesValue()));
    m_FeaturesPollingTimer->start(500);
}

ControllerTreeWindow::~ControllerTreeWindow()
{
    if (VmbErrorSuccess == getTreeStatus())
        m_FeaturesPollingTimer->stop();
    resetControl();
}

void ControllerTreeWindow::showEvent(QShowEvent *event)
{
    updateRegisterFeature();
    if (m_sCurrentSelectedFeature != "")
    {
        FeaturePtr pFeature = getFeaturePtr( m_sCurrentSelectedFeature );
        /* make sure to update the info */
        mapInformation(m_sCurrentSelectedFeature, pFeature);
        showIt(this->currentIndex(), "Description");
    }
}

void ControllerTreeWindow::hideEvent(QHideEvent *event)
{
    updateUnRegisterFeature();
    this->setToolTip("");
    emit setDescription("");
}


void ControllerTreeWindow::saveFeaturesToTextFile ( const QString &sDestPathAndFileName )
{
    QFile file(sDestPathAndFileName);
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream outputstream (&file);

    QMap<QString, FeaturePtr>::iterator i;

    for (i = m_featPtrMap.begin(); i != m_featPtrMap.end(); ++i)
    {
        QString sFeatureDisplayName = i.key();
        outputstream << sFeatureDisplayName << " = " << getFeatureValue ( i.value()) << "\n";
    }

    file.close();
}

void ControllerTreeWindow::setAdjustPacketSizeTimeout()
{
    m_bIsTimeout = true;
}

void ControllerTreeWindow::pollingFeaturesValue()
{
    for (int i = 0; i < m_FeaturesPollingName.size(); i++)
    {
        FeaturePtr FeatPtr = getFeaturePtr(m_FeaturesPollingName.at(i));

        VmbUint32_t nPollValue = 0;
        if (VmbErrorSuccess == FeatPtr->GetPollingTime( nPollValue ))
        {
            if(0 == nPollValue)
            {
                updateExpandedTreeValue(FeatPtr, m_FeaturesPollingName.at(i));
            }
        }
    }
}

bool ControllerTreeWindow::isGigE() const
{
    return (m_bIsGigE && m_bAutoAdjustPacketSize);
}

VmbError_t ControllerTreeWindow::getTreeStatus() const
{
    return m_ListTreeError;
}

MultiCompleter *ControllerTreeWindow::getListCompleter() const
{
    return m_StringCompleter;
}

void ControllerTreeWindow::showTooltip ( const bool bIsChecked )
{
    m_bIsTooltipOn = bIsChecked;
    setToolTip( m_bIsTooltipOn ? m_sTooltip : "" );
}

void ControllerTreeWindow::mapInformation(const QString sName, const FeaturePtr &featPtr)
{
    if( NULL == featPtr)
        return;

    std::string sDescription;
    featPtr->GetDescription(sDescription);

    QString sQDescription = "<br/>";
    sQDescription.append(QString::fromUtf8(sDescription.c_str()));
    sQDescription = sQDescription.replace(".", ".<br/>");

    QString sInfo = getFeatureInformation(featPtr);
    if(sInfo.isEmpty())
    {
        m_DescriptonMap[sName] = sQDescription;
    }
    else
    {
        if(sDescription.empty())
            m_DescriptonMap[sName] = "<b>DESCRIPTION:</b> N/A<br/>" + sInfo;
        else
            m_DescriptonMap[sName] = "<b>DESCRIPTION:</b>" + sQDescription.append("<br/>").append(sInfo);
    }
}

QString ControllerTreeWindow::getFeatureInformation(const FeaturePtr &featPtr)
{
    QString sInformation;
    VmbInt64_t nMin = 0, nMax = 0, nInc = 0;
    double dMin = 0, dMax = 0, dInc = 0;

    std::string sFeatureName;
    featPtr->GetName(sFeatureName);
    sInformation.append("<b>FEATURE NAME:</b> ").append(QString::fromStdString(sFeatureName)).append("<br/>");

    VmbFeatureVisibilityType visibility = VmbFeatureVisibilityUnknown;
    featPtr->GetVisibility(visibility);
    sInformation.append("<b>VISIBILITY:</b> ");
    switch(visibility)
    {
    case VmbFeatureVisibilityUnknown:   sInformation.append("UNKNOWN<br/>");   break;
    case VmbFeatureVisibilityBeginner:  sInformation.append("BEGINNER<br/>");  break;
    case VmbFeatureVisibilityExpert:    sInformation.append("EXPERT<br/>");    break;
    case VmbFeatureVisibilityGuru:      sInformation.append("GURU<br/>");      break;
    case VmbFeatureVisibilityInvisible: sInformation.append("INVISIBLE<br/>"); break;
    default: sInformation.append("N/A<br/>"); break;
    }

    /* get feature type and type-specific info */
    VmbFeatureDataType dataType = VmbFeatureDataUnknown;
    VmbFeatureFlagsType flags;
    VmbError_t error = featPtr->GetDataType(dataType);
    if(VmbErrorSuccess == error)
    {
        switch(dataType)
        {
        case VmbFeatureDataInt:
            {
                /* only show range and increment for integer features that might change */
                if ((VmbErrorSuccess == featPtr->GetFlags(flags)) && ((((VmbFeatureFlagsVolatile|VmbFeatureFlagsWrite|VmbFeatureFlagsModifyWrite) & flags)!=0)||(VmbFeatureFlagsRead == flags)))
                {
                    if( VmbErrorSuccess == featPtr->GetRange(nMin, nMax))
                        sInformation.append("<b>TYPE:</b> Integer<br/><b>MINIMUM:</b> ").append(QString::number(nMin)).append("<br/><b>MAXIMUM:</b> ").append(QString::number(nMax).append("<br/>"));
                    if( (VmbErrorSuccess == featPtr->GetIncrement(nInc)) && (1!=nInc))
                        sInformation.append("<b>INTERVAL:</b> ").append(QString::number(nInc)).append("<br/>");
                }
                break;
            }
        case VmbFeatureDataFloat:
            {
                /* only show range and increment for float features that might change */
                VmbFeatureFlagsType flags;
                if ((VmbErrorSuccess == featPtr->GetFlags(flags)) && ((((VmbFeatureFlagsVolatile|VmbFeatureFlagsWrite|VmbFeatureFlagsModifyWrite) & flags)!=0)||(VmbFeatureFlagsRead == flags)))
                {
                    if( VmbErrorSuccess == featPtr->GetRange(dMin, dMax))
                        sInformation.append("<b>TYPE:</b> Float<br/><b>MINIMUM:</b> ").append(QString::number(dMin,'g',9)).append("<br/><b>MAXIMUM:</b> ").append(QString::number(dMax,'g',12)).append("<br/>");

                    if( VmbErrorSuccess == featPtr->GetIncrement(dInc))
                        sInformation.append("<b>INTERVAL:</b> ").append(QString::number(dInc,'f',10)).append("<br/>");
                }
                break;
            }
        case VmbFeatureDataEnum:    sInformation.append("<b>TYPE:</b> Enumeration<br/>"); break;
        case VmbFeatureDataString:  sInformation.append("<b>TYPE:</b> String<br/>");      break;
        case VmbFeatureDataBool:    sInformation.append("<b>TYPE:</b> Boolean<br/>");     break;
        case VmbFeatureDataCommand: sInformation.append("<b>TYPE:</b> Command<br/>");     break;
        case VmbFeatureDataRaw:     sInformation.append("<b>TYPE:</b> Raw<br/>");     break;
        default: break;
        }
    }

    std::string sCategory;
    featPtr->GetCategory(sCategory);
    sInformation.append("<b>CATEGORY:</b> ").append(QString::fromStdString(sCategory)).append("<br/>");

    FeaturePtrVector           featPtrVec;

    sInformation.append("<br/><b>AFFECTED FEATURE(S):</b> ");
    featPtr->GetAffectedFeatures(featPtrVec);
    for(unsigned int i=0; i < featPtrVec.size(); i++)
    {
        std::string sName;
        featPtrVec.at(i)->GetName(sName);

        if(0 == i)
            sInformation.append("<br/>");

        sInformation.append(QString::fromStdString(sName));
        if(i+1 != featPtrVec.size())
        {
            sInformation.append(", ");
            if(0 == ((i+1) % 4) && (i != 0) )
                sInformation.append("<br/>");
        }
    }

    if(0 == featPtrVec.size())
        sInformation.append("N/A");

    sInformation.append("<br/>");
    return sInformation;
}

void ControllerTreeWindow::onClicked ( const QModelIndex & current )
{
    setCursor(Qt::ArrowCursor);
    scrollTo(current, QAbstractItemView::EnsureVisible);

    if( m_bIsBusy )
        return;

    m_bIsBusy = true;

    if( !current.isValid())
    {
        m_bIsBusy = false;
        return;
    }

    /* check what feature is that */
    FeaturePtr FeatPtr;
    QString sFeature = getFeatureName(current);
    FeatPtr = getFeaturePtr(sFeature);

    /* make sure to update the info */
    mapInformation(sFeature, FeatPtr);
    showIt(current, "Description");

    if (( NULL == FeatPtr) || !isFeatureWritable(sFeature) || 0 == current.column())
    {
        m_bIsBusy = false;
        m_sCurrentSelectedFeature = sFeature;
        resetControl();
        return;
    }

    VmbFeatureDataType dataType = VmbFeatureDataUnknown;
    if(VmbErrorSuccess == FeatPtr->GetDataType(dataType))
    {
        switch(dataType)
        {
        case VmbFeatureDataInt:     createIntSliderSpinBox(current);   break;
        case VmbFeatureDataFloat:   createFloatSliderEditBox(current); break;
        case VmbFeatureDataEnum:    createEnumComboBox(current);       break;
        case VmbFeatureDataString:  createStringEditBox(current);      break;
        case VmbFeatureDataBool:    createBooleanCheckBox(current);    break;
        case VmbFeatureDataCommand: createCommandButton(current);      break;
        case VmbFeatureDataRaw:     createHexEditor(current);          break;
        default: break;
        }
    }

    m_bIsBusy = false;
}

void ControllerTreeWindow::collapse ( const QModelIndex & index )
{
    QVariant varData = index.data();
    int nrow = 0;
    QVariant varChild = 1;

    while(0 != varChild.type())
    {
        QModelIndex child = index.child(nrow++, 0);

        if (0 == child.data().type())
            break;

        varChild = child.data();

        if ( !isEventFeature(getFeaturePtr( varChild.toString() )))
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

void ControllerTreeWindow::expand( const QModelIndex & index )
{
    QVariant varData = index.data();
    int nrow = 0;
    QVariant varChild = 1;

    while(0 != varChild.type())
    {
        QModelIndex child = index.child(nrow++, 0);
        if (0 == child.data().type())
            break;
        varChild = child.data();

        if(isExpanded(child))
            expand(child);

        FeaturePtr pFeature = getFeaturePtr( varChild.toString() );

        if( !isEventFeature( pFeature ))
        {
            if ( !SP_ISNULL( pFeature ))
            {
                VmbFeatureFlagsType flags       = (VmbFeatureFlagsType)0UL;
                VmbUint32_t         pollingTime = 0;
                if (    VmbErrorSuccess == pFeature->GetFlags( flags )
                     && VmbFeatureFlagsVolatile & flags
                     && VmbErrorSuccess == pFeature->GetPollingTime( pollingTime )
                     && 0 == pollingTime )
                     m_FeaturesPollingName.push_back( varChild.toString() );
                else
                    registerFeatureObserver( varChild.toString() );
            }
        }
    }
}

void ControllerTreeWindow::updateColWidth()
{
    resizeColumnToContents(0);
    resizeColumnToContents(1);
}

void ControllerTreeWindow::updateRegisterFeature()
{
    for(int i=0; i < model()->rowCount(); i++)
    {
        if (isExpanded(model()->index(i, 0)))
        {
            expand(model()->index(i, 0));
        }
    }

    updateColWidth();
}

void ControllerTreeWindow::updateUnRegisterFeature()
{
    for(int i=0; i < model()->rowCount(); i++)
    {
        collapse(model()->index(i, 0));
    }
}

bool ControllerTreeWindow::isEventFeature( const FeaturePtr pFeature ) const
{
    std::string sCategory;
    if (    !SP_ISNULL( pFeature )
         && VmbErrorSuccess == SP_ACCESS( pFeature )->GetCategory( sCategory )
         && std::strstr( sCategory.c_str(), "/EventID" ))
    {
        return true;
    }
    else
    {
        return false;
    }
}

void ControllerTreeWindow::setupTree()
{
    m_ModelGuru = new QStandardItemModel();
    m_Model = new QStandardItemModel();
    QStringList sHeader;
    sHeader << tr("Feature ") << tr("Value ");
    m_Model->setHorizontalHeaderLabels( sHeader );
    m_ModelGuru->setHorizontalHeaderLabels( sHeader );
}

bool  ControllerTreeWindow::findCategory ( const QMap <QString, QString>& map, const QString& sName ) const
{
    QMap<QString, QString>::const_iterator itr = map.find(sName);
    while (itr != map.constEnd())
    {
        if(0 == itr.key().compare(sName))
        {
            return true;
        }

        ++itr;
    }

    return false;
}

void ControllerTreeWindow::sortCategoryAndAttribute ( const FeaturePtr &featPtr, QStandardItemModel *Model )
{
    QFont categoryFont;
    categoryFont.setBold(true);

    QList<QStandardItem *> items;
    QList<QStandardItem *> pParent;
    QString sLastNode;
    bool bHasLastNode = false;
    int nLevel = 0;

    std::string sCategory, sFeatureName;
    QString sFeatureValue;
    bool bIsWritable = false;
    VmbFeatureVisibilityType visibilityType;
    VmbError_t error;

    error = featPtr->GetDisplayName(sFeatureName);

    if( VmbErrorSuccess == featPtr->GetCategory(sCategory) &&
        VmbErrorSuccess == error )
    {
        /* TODO check return value */
        sFeatureValue = getFeatureValue(featPtr);
        error = featPtr->IsWritable(bIsWritable);
        featPtr->GetVisibility(visibilityType);

        /* is it a GigE? : temporarily use to handle floating gain */
        if(0 == sCategory.compare("/GigE"))
        {
            m_bIsGigE = true;
        }

        /* Feature directly in root category */
        if ( 0 == sCategory.length() )
        {
            sCategory = "/";
        }
    }
    else
    {
        return;
    }

    QString sPath = QString::fromStdString(sCategory);

    int nCount =  sPath.count('/');

    while((nLevel < nCount))
    {
        nLevel++;
        QString sPart = sPath.section('/', nLevel, nLevel);
        /* Feature directly in root category */
        if ( 0 == sPart.length() )
        {
            sPart = "/";
        }
        bool bNodeExists = false;
        if( 0 < sPart.length())
        {
            /* check if category in level already available */
            if(!findCategory ( m_Level.at(nLevel-1), sPart ))
            {
                bNodeExists = false;
                switch(nLevel-1)
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
            if( !bNodeExists )
            {
                QList<QStandardItem *> currentItem;

                currentItem << new QStandardItem(sPart) << new QStandardItem("");
                currentItem.at(0)->setEditable(false);
                currentItem.at(1)->setEditable(false);

                if(bHasLastNode)
                {
                    /* get the pointer of list items */
                    items = Model->findItems(sLastNode, Qt::MatchRecursive);
                    if(0 != items.size())
                    {
                        /* find the right parent level, and add the attribute item there*/
                        QString sGrandParent = sPath.section('/', nLevel-1, nLevel-1);
                        items.at(getGrandParentLevel(items, sGrandParent))->appendRow(currentItem);
                        sLastNode = sPart;
                        continue;
                    }
                }

                if(pParent.isEmpty())
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
                if(0 != items.size())
                {
                    /* find the right parent level, and add the attribute item there*/
                    QString sGrandParent = sPath.section('/', nLevel-1, nLevel-1);
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
    items = Model->findItems(sLastNode, Qt::MatchWrap|Qt::MatchRecursive);

    if(0 != items.size() && (VmbFeatureVisibilityInvisible != visibilityType))
    {
        QList<QStandardItem *> attributeItems;

        std::string sStdName;
        featPtr->GetName(sStdName);
        QString sName = QString::fromStdString(sStdName);
        std::string sRepresentation;
        featPtr->GetRepresentation(sRepresentation);

        if( Helper::needsIPv4Format(sName, sRepresentation))
        {
           sFeatureValue = Helper::displayValueToIPv4(sFeatureValue);
        }

        attributeItems << new QStandardItem(QString::fromStdString(sFeatureName)) << new QStandardItem(sFeatureValue);

        attributeItems.at(0)->setEditable(false);
        attributeItems.at(1)->setEditable(false);

        bIsWritable ? attributeItems.at(0)->setForeground(QColor(0,128, 0))/* green */ : attributeItems.at(0)->setForeground(QColor(135,135, 135)) /* grey */;

        /* find the right parent level, and add the attribute item there*/
        QString sGrandParent = sPath.section('/', nCount-1, nCount-1);
        items.at(getGrandParentLevel(items, sGrandParent))->appendRow(attributeItems);

        /* register Events */
        if( isEventFeature( featPtr ))
        {
            featPtr->RegisterObserver(m_pFeatureObs);
        }
    }
}

unsigned int ControllerTreeWindow::getGrandParentLevel( const QList<QStandardItem *>& items, const QString &sGrandParent) const
{
    unsigned int nLevel = 0;
    for (int i=0; i<items.size(); i++ )
    {
        QStandardItem *parent = items.at(i)->parent();

        if(0 == parent)
            continue;

        if(0 == sGrandParent.compare(parent->text()) )
        {
            nLevel = i;
            break;
        }
    }

    return nLevel;
}

bool ControllerTreeWindow::registerFeatureObserver ( const QString &sFeatureName )
{
    /* register all features to the observer, so you will get notification in case any features value has been changed */
    QMap<QString, FeaturePtr>::iterator find_pos = m_featPtrMap.find( sFeatureName);
    if( find_pos != m_featPtrMap.end() )
    {
        VmbError_t error = find_pos.value()->RegisterObserver(m_pFeatureObs);
        if( VmbErrorSuccess != error)
        {
            emit logging( "ERROR Register Observer - Feature: " + sFeatureName + " returned "+QString::number(error)+
                            " "+ Helper::mapReturnCodeToString(error)) ;
        }
        updateExpandedTreeValue(find_pos.value(), sFeatureName);
        return true;
    }
    return false;
}

void ControllerTreeWindow::unregisterFeatureObserver ( const QString &sFeatureName )
{
    QMap<QString, FeaturePtr>::iterator find_pos = m_featPtrMap.find( sFeatureName);

    if( find_pos != m_featPtrMap.end() )
    {
        VmbError_t error = find_pos.value()->UnregisterObserver(m_pFeatureObs);
        if( VmbErrorSuccess != error)
        {
            if(VmbErrorNotFound != error )
                emit logging( "ERROR Unregister Observer - Feature: " + find_pos.key() + " returned "+QString::number(error)+" "+
                                Helper::mapReturnCodeToString(error)) ;
        }
    }
}

void ControllerTreeWindow::updateExpandedTreeValue ( const FeaturePtr &featPtr, const QString &sName )
{
    QString sValue = getFeatureValue(featPtr);
    bool bIsWritable = false;
    if( VmbErrorSuccess == featPtr->IsWritable(bIsWritable) )
        onSetChangedFeature(sName, sValue, bIsWritable);
}

void ControllerTreeWindow::onSetEventMsg ( const QStringList &sMsg )
{
    setEventMessage(sMsg);
}

/* synchronize the event information in tree and event window
   make sure to collect last information for event window when acquisition stops */
void ControllerTreeWindow::synchronizeEventFeatures()
{
    if(m_bIsGigE)
    {
        FeatureObserverPtr p = SP_DYN_CAST(m_pFeatureObs,FeatureObserver);
        if( NULL != p)
        {
            if(SP_ACCESS(p)->isEventRunning())
            {
                SP_ACCESS(p)->startObserverTimer();
            }
        }
    }
}

/* put the value to related features changed
   keep tree features value up-to-date  */
void ControllerTreeWindow::onSetChangedFeature ( const QString &sFeature, const QString &sValue, const bool &bIsWritable )
{
    QModelIndexList currentItems = m_Model->match( m_Model->index(0,0), Qt::DisplayRole, QVariant::fromValue(sFeature), 1, Qt::MatchWrap|Qt::MatchRecursive);

    if( 0 == currentItems.size() )
        return;

    QString sFeatureValue = sValue;

    if( Helper::needsIPv4Format(sFeature) )
    {
        sFeatureValue = Helper::displayValueToIPv4(sValue);
    }
    else if( sFeature == "Device MAC Address")
    {
        sFeatureValue = Helper::displayValueToMAC(sValue);
    }

    QModelIndex newIndex = m_Model->index( currentItems.at(0).row(), 1, currentItems.at(0).parent());
    m_Model->setData( newIndex, QVariant(sFeatureValue), Qt::EditRole );

    //TLParamsLock?
    QList<QStandardItem *> itemsCol1 = m_Model->findItems(sFeature, Qt::MatchWrap|Qt::MatchRecursive, 0) ;

    if(!itemsCol1.empty())
    {
        bIsWritable ? itemsCol1.at(0)->setForeground(QColor(0,128, 0))/*green*/ : itemsCol1.at(0)->setForeground(QColor(135,135, 135))/*grey*/;
        if( 0 == m_sCurrentSelectedFeature.compare(sFeature))
            updateWidget(bIsWritable, QVariant(sFeatureValue));
    }

    if ( !m_FeaturePtr_EnumComboBox || !m_EnumComboBox )
        return;

    /* update Exposure-/Gain-/BalanceWhite- Auto when "Once" clicked */
    if(     Helper::isAutoFeature( m_sCurrentSelectedFeature)
        &&  Helper::isAutoFeature( sFeature)
        &&  m_sCurrentSelectedFeature == sFeature
        &&  sFeatureValue == "Off"
        && m_EnumComboBox->currentText() == "Once" )
    {
        const int nIndex = m_EnumComboBox->findText("Off");
        if(-1 != nIndex)
            m_EnumComboBox->setCurrentIndex(nIndex);
    }

    if( m_sCurrentSelectedFeature == sFeature && sValue.isEmpty() )
    {
        m_EnumComboBox->setItemText(m_EnumComboBox->count(), sValue);
        m_EnumComboBox->setCurrentIndex(m_EnumComboBox->count());
    }
}

template <typename COMPOUND_WIDGET>
void updateEditWidget(COMPOUND_WIDGET *w, const QVariant &value, bool bIsWritable)
{
    w->setEnabled(bIsWritable);
    if(bIsWritable)
    {
        QLineEdit * pChild= w-> template findChild<QLineEdit *>(QString("value"));
        if (    NULL != pChild
            &&  !pChild->isModified()
            )
        {
            const QString newValue = value.toString();
            if( pChild->text() != newValue)
            {
                pChild->setText(newValue);
            }
        }
    }
}

void ControllerTreeWindow::updateWidget ( const bool bIsWritable, const QVariant &value )
{
    if( NULL != m_IntSpinSliderWidget )
    {
        m_IntSpinSliderWidget->setEnabled(bIsWritable);
        if(bIsWritable)
        {
            IntSpinBox * spinBox = m_IntSpinSliderWidget->findChild<IntSpinBox *>(QString("value"));
            if(     NULL != spinBox )
            {
                int newValue = value.toInt();
                if(     spinBox->value() != newValue
                    &&  !spinBox->hasFocus())
                {
                    spinBox->setValue( newValue );
                }
            }
        }
    }

    else if( NULL != m_ButtonWidget )
        m_ButtonWidget->setEnabled(bIsWritable);

    else if( NULL != m_ComboWidget )
        m_ComboWidget->setEnabled(bIsWritable);

    else if( NULL != m_FloatSliderEditWidget )
    {
        updateEditWidget( m_FloatSliderEditWidget, value, bIsWritable );
    }

    else if( NULL != m_BooleanWidget )
    {
        m_BooleanWidget->setEnabled(bIsWritable);
        if(bIsWritable)
            m_BooleanWidget->findChild<QCheckBox *>(QString("value"))->setChecked(value.toBool());
    }

    else if( NULL != m_EditWidget )
    {
        updateEditWidget( m_EditWidget, value, bIsWritable );
    }

    else if( NULL != m_LineEditWidget )
    {
        updateEditWidget(m_LineEditWidget, value, bIsWritable );
    }

    else if( NULL != m_LogSliderWidget )
    {
        updateEditWidget( m_LogSliderWidget, value, bIsWritable );
    }
}

QString ControllerTreeWindow::getFeatureValue ( const FeaturePtr &featPtr ) const
{
    VmbInt64_t  nValue64    = 0;
    double      dValue      = 0;
    bool        bValue      = false;

    std::string stdValue;
    QString     sValue("");

    VmbFeatureDataType dataType = VmbFeatureDataUnknown;

    VmbError_t error = featPtr->GetDataType(dataType);

    if(VmbErrorSuccess == error)
    {
        switch(dataType)
        {
            case VmbFeatureDataInt:
                if(VmbErrorSuccess == featPtr->GetValue(nValue64))
                    sValue = QString::number(nValue64);
                break;

            case VmbFeatureDataFloat:
                if(VmbErrorSuccess == featPtr->GetValue(dValue))
                    sValue = QString::number(dValue);
                break;

            case VmbFeatureDataEnum:
                if(VmbErrorSuccess == featPtr->GetValue(stdValue))
                    sValue = QString::fromStdString(stdValue);
                break;

            case VmbFeatureDataString:
                if(VmbErrorSuccess == featPtr->GetValue(stdValue))
                    sValue = QString::fromStdString (stdValue);
                break;

            case VmbFeatureDataBool:
                if(VmbErrorSuccess == featPtr->GetValue(bValue))
                    bValue ? sValue = "true" : sValue = "false";
                break;

            case VmbFeatureDataCommand:
                sValue = "[COMMAND]";
                break;

            case VmbFeatureDataRaw:
                sValue = "Click here to open";
                break;
            default: break;
        }
    }
    return sValue;
}

QString ControllerTreeWindow::getFeatureName ( const QModelIndex& item ) const
{
    QString sAttrName("");
    if(item.isValid())
    {
        QModelIndex indexFirstCol = item.sibling(item.row(), 0);
        sAttrName = indexFirstCol.model()->data( indexFirstCol, Qt::DisplayRole ).toString();
    }
    return sAttrName;
}

FeaturePtr ControllerTreeWindow::getFeaturePtr ( const QString &sFeature )
{
    QMap<QString, FeaturePtr>::iterator i = m_featPtrMap.find(sFeature);

    if ( m_featPtrMap.end() != i )
    {
        return i.value();
    }
    else
    {
        return FeaturePtr();
    }
}

void ControllerTreeWindow::showIt ( const QModelIndex item, const QString &sWhat )
{
    QString sAttrName = getFeatureName(item);

    QMap<QString, QString>::const_iterator i;

    if( 0 == sWhat.compare("Description"))
        i = m_DescriptonMap.find(sAttrName);

    while (i != m_DescriptonMap.constEnd())
    {
        if(0 == i.key().compare(sAttrName))
        {
            m_sTooltip = i.value();
            if(m_bIsTooltipOn)
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

QString ControllerTreeWindow::getFeatureNameFromMap ( const QString &sDisplayName ) const
{
    QString sFeature;
    QMap<QString, QString>::const_iterator itr = m_DisplayFeatureNameMap.find(sDisplayName);
    if (itr != m_DisplayFeatureNameMap.constEnd())
    {
        sFeature = itr.value();
    }

    return sFeature;
}

bool ControllerTreeWindow::isFeatureWritable ( const QString & sFeature )
{
    bool bIsWritable = false;
    FeaturePtr FeatPtr;

    FeatPtr = getFeaturePtr(sFeature);
    if(FeaturePtr() == FeatPtr)
        return bIsWritable;

    FeatPtr->IsWritable(bIsWritable);

    return bIsWritable;
}

/* VmbFeatureDataRaw */
void ControllerTreeWindow::createHexEditor ( const QModelIndex item )
{
    FeaturePtr FeatPtr;
    QString sFeature = getFeatureName(item);

    bool bIsReadOnly = true;
    if (isFeatureWritable(sFeature))
    {
        bIsReadOnly = false;
    }
    FeatPtr = getFeaturePtr(sFeature);
    if( NULL == FeatPtr)
        return;

    if( VmbErrorSuccess == FeatPtr->GetValue(m_RawData) )
    {
        m_HexWindow = new HexMainWindow(this, NULL, "Raw Data Editor", bIsReadOnly, FeatPtr);
        unsigned char * c = &(m_RawData[0]);
        QByteArray data = QByteArray::fromRawData((char*)c, m_RawData.size());
        m_HexWindow->setData(data);
    }
}

/* VmbFeatureDataCommand */
void ControllerTreeWindow::createCommandButton ( const QModelIndex item )
{
    QString sFeature_Command = getFeatureName(item);

    if (isFeatureWritable(sFeature_Command))
    {
        m_sCurrentSelectedFeature = m_sFeature_Command = sFeature_Command;

        m_FeaturePtr_Command = getFeaturePtr(sFeature_Command);
        if(NULL == m_FeaturePtr_Command)
            return;

        resetControl();

        m_ButtonWidget  = new QWidget(this);
        m_HButtonLayout = new QHBoxLayout(m_ButtonWidget);
        m_CmdButton = new QPushButton(QIcon(":/VimbaViewer/Images/execute.png"), QString("Execute..."), m_ButtonWidget);

        m_HButtonLayout2 = new QHBoxLayout();
        m_HButtonLayout2->addWidget(m_CmdButton);
        m_HButtonLayout->addLayout(m_HButtonLayout2);

        QPoint p = QCursor::pos();
        m_ButtonWidget->setFixedSize(m_sCurrentSelectedFeature.length()*10, 60);
        m_ButtonWidget->setWindowFlags( Qt::Tool | Qt::WindowCloseButtonHint );
        m_ButtonWidget->setWindowTitle(m_sCurrentSelectedFeature);
        AdjustOffscreenPosition(p, *m_ButtonWidget );
        m_ButtonWidget->move(p.x(), p.y());
        m_ButtonWidget->show();

        QObject::connect( m_CmdButton, SIGNAL(clicked()), this, SLOT(onCmdButtonClick()) );
        setCursor(Qt::PointingHandCursor);
    }
}

/* run command when feature button clicked */
void ControllerTreeWindow::onCmdButtonClick()
{
    VmbError_t error;

    if(     (0 == m_sFeature_Command.compare("AcquisitionStop"))
        ||  (0 == m_sFeature_Command.compare("Acquisition Stop"))
        ||  (0 == m_sFeature_Command.compare("AcquisitionAbort"))
        ||  (0 == m_sFeature_Command.compare("Acquisition Abort")))
    {
        error = m_FeaturePtr_Command->RunCommand();
        emit acquisitionStartStop("AcquisitionStopFreerun");
    }
    else if(        (0 == m_sFeature_Command.compare("AcquisitionStart"))
                ||  (0 == m_sFeature_Command.compare("Acquisition Start")))
    {
        emit acquisitionStartStop("AcquisitionStartFreerun");
    }

    if(     (0 != m_sFeature_Command.compare("AcquisitionStop"))
        &&  (0 != m_sFeature_Command.compare("Acquisition Stop"))
        &&  (0 != m_sFeature_Command.compare("AcquisitionAbort"))
        &&  (0 != m_sFeature_Command.compare("Acquisition Abort")))
    {
        error = m_FeaturePtr_Command->RunCommand();
    }

    if( VmbErrorSuccess != error)
    {
        emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " returned "+QString::number(error)+
                        " "+ Helper::mapReturnCodeToString(error)) ;
    }
    else if( 0 == m_sFeature_Command.compare("GVSP Adjust Packet Size"))
    {
        bool bIsDone = false;
        this->setEnabled(false);
        emit enableViewerMenu (false);
        QPixmap Pixmap( ":/VimbaViewer/Images/refresh.png" );
        

        while((!bIsDone) && (VmbErrorSuccess == error))
        {
            error = m_FeaturePtr_Command->IsCommandDone(bIsDone);
        }

        this->setEnabled(true);
        emit enableViewerMenu (true);
    }
}

/* VmbFeatureDataEnum */
void ControllerTreeWindow::createEnumComboBox ( const QModelIndex item )
{
    VmbError_t error;
    QString sFeature_EnumComboBox = getFeatureName(item);

    // Don't create drop down for read only features
    if (isFeatureWritable(sFeature_EnumComboBox))
    {
        m_sCurrentSelectedFeature = m_sFeature_EnumComboBox = sFeature_EnumComboBox;

        // Set FeaturePtr member to currently selected enum feature
        m_FeaturePtr_EnumComboBox = getFeaturePtr(m_sFeature_EnumComboBox);
        if(FeaturePtr() == m_FeaturePtr_EnumComboBox)
            return;

        resetControl();
        m_ComboWidget   = new QWidget(this);
        m_HComboLayout  = new QHBoxLayout(m_ComboWidget);
        m_EnumComboBox  = new ExComboBox( );
        m_HComboLayout2 = new QHBoxLayout();
        m_HComboLayout2->addWidget(m_EnumComboBox);
        m_HComboLayout->addLayout(m_HComboLayout2);

        // Get all possible enumerations as string
        StringVector sEnumEntries;
        error = m_FeaturePtr_EnumComboBox->GetValues(sEnumEntries);

        if(VmbErrorSuccess == error)
        {
            std::string sCurrentValue;
            unsigned int nPos = 0;
            // Get currently selected enumeration
            if(VmbErrorSuccess == m_FeaturePtr_EnumComboBox->GetValue(sCurrentValue) || VmbErrorSuccess == error)
            {
                for(unsigned int i=0; i < sEnumEntries.size(); i++)
                {
                    // Check each enumeration if it is currently applicable
                    bool bIsAvailable = false;
                    m_FeaturePtr_EnumComboBox->IsValueAvailable(sEnumEntries.at(i).c_str(), bIsAvailable );

                    if(bIsAvailable)
                    {
                        // Add all applicable enumerations to drop down
                        m_EnumComboBox->addItem(QString::fromStdString(sEnumEntries.at(i)));
                        // Is this currently selected enumeration?
                        if(0 == sEnumEntries.at(i).compare(sCurrentValue))
                            // Set drop down index
                            m_EnumComboBox->setCurrentIndex(nPos);
                        nPos++;
                    }
                }

                if(sCurrentValue.empty())
                {
                    m_EnumComboBox->setItemText(nPos, QString::fromStdString(sCurrentValue));
                    m_EnumComboBox->setCurrentIndex(nPos);
                }
            }

            QPoint p = QCursor::pos();
            m_ComboWidget->setFixedSize(m_sCurrentSelectedFeature.length()*13, 60);
            m_ComboWidget->setWindowFlags( Qt::Tool | Qt::WindowCloseButtonHint );
            m_ComboWidget->setWindowTitle(m_sCurrentSelectedFeature);
            AdjustOffscreenPosition(p, *m_ComboWidget );
            m_ComboWidget->move(p.x(), p.y());
            m_ComboWidget->show();

            QObject::connect( m_EnumComboBox, SIGNAL(activated(const QString &)), this, SLOT(onEnumComboBoxClick(const QString &)) );
            setCursor(Qt::PointingHandCursor);
        }
        else
        {
            emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (GetEnumRange) returned "+QString::number(error)+
                " "+ Helper::mapReturnCodeToString(error)) ;
        }
    }
}

void ControllerTreeWindow::onEnumComboBoxClick ( const QString &sSelected )
{
    std::string sCurrentValue;
    VmbError_t error;
    int nIndex = 0;
    int nIndexOld = 0;

    error = m_FeaturePtr_EnumComboBox->GetValue(sCurrentValue);
    if( VmbErrorSuccess == error )
    {
        nIndexOld = m_EnumComboBox->findText(QString::fromStdString(sCurrentValue));
    }
    else
    {
        emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (GetEnumValue) " + sSelected +" returned "+QString::number(error)+
                      " "+ Helper::mapReturnCodeToString(error)) ;
    }

    if( (0 == m_sFeature_EnumComboBox.compare("PixelFormat")) || (0 == m_sFeature_EnumComboBox.compare("Pixel Format")) ||
        (0 == m_sFeature_EnumComboBox.compare("AcquisitionMode")) || (0 == m_sFeature_EnumComboBox.compare("Acquisition Mode")) )
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
        if(VmbErrorSuccess != error)
        {
            emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (SetEnumValue) " + sSelected +" returned "+QString::number(error)+
                          " "+ Helper::mapReturnCodeToString(error)) ;
        }

        if(VmbErrorSuccess == m_FeaturePtr_EnumComboBox->GetValue(sCurrentValue))
        {
            /* make sure to set back the valid value from the camera to combobox */
            nIndex = m_EnumComboBox->findText(QString::fromStdString(sCurrentValue));
            m_EnumComboBox->setCurrentIndex(nIndex);
        }
        else
        {
            m_EnumComboBox->setCurrentIndex(nIndexOld);
            {
                emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (GetEnumValue) " + sSelected +" returned "+QString::number(error)+
                              " "+ Helper::mapReturnCodeToString(error)) ;
            }
        }

#ifdef WIN32
        ::Sleep(5);
#else
        ::usleep(5000);
#endif

        /*start Acquisition */
        emit acquisitionStartStop("AcquisitionStart");
        return;
    }

    error = m_FeaturePtr_EnumComboBox->SetValue(sSelected.toUtf8().data());

    if(VmbErrorSuccess != error)
    {
        emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (SetEnumValue) " + sSelected +" returned "+QString::number(error)+
                      " "+ Helper::mapReturnCodeToString(error)) ;
    }

    if( VmbErrorSuccess == m_FeaturePtr_EnumComboBox->GetValue(sCurrentValue) )
    {
        /* make sure to set back the valid value from the camera to combobox */
        nIndex = m_EnumComboBox->findText(QString::fromStdString(sCurrentValue));
        m_EnumComboBox->setCurrentIndex(nIndex);
    }
    else
    {
        m_EnumComboBox->setCurrentIndex(nIndexOld);
        emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (GetEnumValue) " + sSelected +" returned "+QString::number(error)+
                      " "+ Helper::mapReturnCodeToString(error)) ;
    }
}

/* VmbFeatureDataInt */
void ControllerTreeWindow::createIntSliderSpinBox ( const QModelIndex item )
{
    VmbError_t error;
    QString sFeature_IntSpinBox   = getFeatureName(item);

    if (isFeatureWritable(sFeature_IntSpinBox))
    {
        m_sCurrentSelectedFeature = m_sFeature_IntSpinBox = sFeature_IntSpinBox;

        m_FeaturePtr_IntSpinBox = getFeaturePtr(m_sFeature_IntSpinBox);

        std::string sRepresentation;
        m_FeaturePtr_IntSpinBox->GetRepresentation(sRepresentation);

        VmbInt64_t nMin = 0, nMax = 0, nInc = 1, nValue = 0;
        error = m_FeaturePtr_IntSpinBox->GetRange(nMin, nMax);

        /* use a line edit to show reg add in Hex */
        QString sName = getFeatureNameFromMap(m_sCurrentSelectedFeature);
        if( VmbErrorSuccess == error &&
                (   sRepresentation == "HexNumber" ||
                    Helper::needsIPv4Format(sName, sRepresentation) ||
                    nMax > 9999
                )
          )
        {
            createLineEdit(item);
            return;
        }

        if(NULL == m_FeaturePtr_IntSpinBox)
            return;

        resetControl();

        m_IntSpinSliderWidget = new QWidget(this);
        m_HSpinSliderLayout_Int = new QHBoxLayout( m_IntSpinSliderWidget );

        if(VmbErrorSuccess == error)
        {
            if( nMin < std::numeric_limits<int>::min() )
            {
                nMin = std::numeric_limits<int>::min();
            }

            if( nMax > std::numeric_limits<int>::max() )
            {
                nMax = std::numeric_limits<int>::max();
                if(VmbErrorSuccess == m_FeaturePtr_IntSpinBox->GetValue(nValue))
                {
                    if( nMax < nValue )
                        m_FeaturePtr_IntSpinBox->SetValue(nMax);
                }
            }

            m_SpinBox_Int = new IntSpinBox(0);
            m_SpinBox_Int->setObjectName("value");      //mark as the element of this dialog that contains the value, used in "updateWidget()"
            m_Slider_Int = new QSlider(Qt::Horizontal);
            m_Slider_Int->installEventFilter(this);
            m_Slider_Int->setTickPosition(QSlider::TicksBelow);
            error = m_FeaturePtr_IntSpinBox->GetIncrement(nInc);
            if(VmbErrorSuccess == error)
            {
                m_SpinBox_Int->setSingleStep(nInc);
                m_Slider_Int->setTickInterval((nMax-nMin)/6/*nInc*/);
                m_Slider_Int->setSingleStep(nInc);
            }

            nMax = nMax - ((nMax-nMin) % nInc);
            m_SpinBox_Int->setRange(nMin, nMax);
            m_Slider_Int->setRange( nMin, nMax);

            m_nSliderStep = nInc;

            error = m_FeaturePtr_IntSpinBox->GetValue(nValue);
            if(VmbErrorSuccess == error)
            {
                if( nValue > std::numeric_limits<int>::max() )
                {
                    nValue = std::numeric_limits<int>::max();
                }

                m_SpinBox_Int->setValue(nValue);
                m_Slider_Int->setValue(nValue);
                m_nIntSliderOldValue = nValue;
            }

            m_Slider_Int->setPageStep((nMax-nMin)/5/nInc);
        }
        else
        {
            emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (GetRange) returned "+QString::number(error)+
                          " "+ Helper::mapReturnCodeToString(error)) ;
        }

        m_HSpinSliderLayout_Int2 = new QHBoxLayout( );
        m_HSpinSliderLayout_Int2->addWidget(m_Slider_Int);
        m_HSpinSliderLayout_Int2->addWidget(m_SpinBox_Int);
        m_HSpinSliderLayout_Int->addLayout(m_HSpinSliderLayout_Int2);

        QPoint p = QCursor::pos();
        m_IntSpinSliderWidget->setFixedHeight(60);
        m_IntSpinSliderWidget->setWindowFlags( Qt::Tool | Qt::WindowCloseButtonHint );
        m_IntSpinSliderWidget->setWindowTitle(m_sCurrentSelectedFeature);
        AdjustOffscreenPosition(p, *m_IntSpinSliderWidget );
        m_IntSpinSliderWidget->move(p.x(), p.y());
        m_IntSpinSliderWidget->show();

        connect( m_Slider_Int,  SIGNAL(valueChanged(int)), this, SLOT(onIntSliderChanged(int)) );
        connect( m_SpinBox_Int, SIGNAL(editingFinished()), this, SLOT(onIntSpinBoxClick()) );
        setCursor(Qt::PointingHandCursor);
    }
}

void ControllerTreeWindow::onIntSliderReleased()
{
    onIntSliderChanged(m_Slider_Int->value());
}

void ControllerTreeWindow::onIntSliderChanged ( int nValue )
{
    /* ignore it while pressing mouse and when slider still busy */
    if(m_bIsMousePressed || !m_bIsJobDone )
      return;

    m_bIsJobDone = false;
    int nCurrentValue = nValue;

    if( (m_nIntSliderOldValue > nValue) && (m_Slider_Int->minimum() != nValue) )
    {
        int nMod = nValue%m_nSliderStep;
        nValue = nValue - nMod;
    }

    if( (0 != (nValue%m_nSliderStep)) && ( m_Slider_Int->minimum() != nValue) && ( m_Slider_Int->maximum() != nValue) )
    {
        nValue = nValue + (m_nSliderStep - (nValue%m_nSliderStep));
    }

    if(m_Slider_Int->hasFocus())
    {
        setIntegerValue (nValue);
    }

    m_nIntSliderOldValue = nCurrentValue;
    m_bIsJobDone = true;
}

void ControllerTreeWindow::onIntSpinBoxClick()
{
    int nValue = m_SpinBox_Int->value();

    if( (0 != (nValue%m_nSliderStep)) && ( m_SpinBox_Int->minimum() != nValue) && ( m_SpinBox_Int->maximum() != nValue) )
    {
        nValue = (nValue + (m_nSliderStep - (nValue%m_nSliderStep)) );
    }

    setIntegerValue (nValue);
}

void ControllerTreeWindow::setIntegerValue ( const int &nValue )
{
    VmbError_t error;

    if( 0 == m_sFeature_IntSpinBox.compare("Width") ||0 == m_sFeature_IntSpinBox.compare("Height") )
    {
        /* stop Acquisition */
        emit acquisitionStartStop("AcquisitionStopWidthHeight");

        error = m_FeaturePtr_IntSpinBox->SetValue(nValue);
        if( VmbErrorSuccess != error )
        {
            emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (SetValue) "+ QString::number(nValue)+" returned "+QString::number(error)+
                          " "+ Helper::mapReturnCodeToString(error)) ;
        }

        updateCurrentIntValue();

        /*start Acquisition */
        emit acquisitionStartStop("AcquisitionStart");
        return;
    }

    error = m_FeaturePtr_IntSpinBox->SetValue(nValue);
    if( VmbErrorSuccess != error )
    {
        emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (SetValue) "+ QString::number(nValue)+" returned "+QString::number(error)+
                      " "+ Helper::mapReturnCodeToString(error)) ;
    }

    updateCurrentIntValue();
}

void ControllerTreeWindow::updateCurrentIntValue()
{
    VmbInt64_t nCurrentValue = 0;

    VmbError_t error = m_FeaturePtr_IntSpinBox->GetValue(nCurrentValue);
    if(VmbErrorSuccess == error)
    {
        /* make sure to set back the valid value from the camera to slider and spinbox */
        m_SpinBox_Int->setValue(nCurrentValue);
        m_Slider_Int->setValue(nCurrentValue);
    }
    else
    {
        emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (GetValue) "+ QString::number(nCurrentValue)+" returned "+QString::number(error)+
                      " "+ Helper::mapReturnCodeToString(error)) ;
    }
}

/* VmbFeatureDataFloat */
void ControllerTreeWindow::createFloatSliderEditBox ( const QModelIndex item )
{
    VmbError_t error;
    QString sFeature_FloatSliderEditBox   = getFeatureName(item);

    if (isFeatureWritable(sFeature_FloatSliderEditBox))
    {
        m_sCurrentSelectedFeature = m_sFeature_FloatSliderSpinBox = sFeature_FloatSliderEditBox;
        m_FeaturePtr_FloatSliderSpinBox = getFeaturePtr(sFeature_FloatSliderEditBox);
        if(NULL == m_FeaturePtr_FloatSliderSpinBox)
            return;

        resetControl();

        error = m_FeaturePtr_FloatSliderSpinBox->GetRange(m_dMinimum, m_dMaximum);

        if(VmbErrorSuccess == error)
        {
            /* if there's an increment, compute the correct maximum (allowing some float uncertainty) */
            if(VmbErrorSuccess == m_FeaturePtr_FloatSliderSpinBox->GetIncrement(m_dIncrement))
            {
                double dSteps = floor((m_dMaximum-m_dMinimum)/m_dIncrement*1.000000000001);
                double dMaximum = m_dMinimum + dSteps*m_dIncrement*1.000000000001;
                if (dMaximum < m_dMaximum)
                    m_dMaximum = dMaximum;
            }
            else
            {
                m_dIncrement = 0.0;
            }
        }
        else
        {
            emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (GetRange) "+" returned "+QString::number(error)+
                " "+ Helper::mapReturnCodeToString(error)) ;
            return;
        }

        std::string sRepresentation;
        m_FeaturePtr_FloatSliderSpinBox->GetRepresentation(sRepresentation);

        /* use a logarithmic slider for exposure */
        if( 0 == sFeature_FloatSliderEditBox.compare("Exposure Time") || 0 == sFeature_FloatSliderEditBox.compare("ExposureTimeAbs")
            || 0 == sRepresentation.compare("Logarithmic"))
        {
            createLogarithmicSlider(item);
            return;
        }

        m_FloatSliderEditWidget = new QWidget(this);
        m_HSpinSliderLayout_Float = new QHBoxLayout( m_FloatSliderEditWidget );

        /* Line Edit */
        m_EditBox_Float = new QLineEdit(0);
        m_EditBox_Float->setObjectName("value");    //mark as the element of this dialog that contains the value, used in "updateWidget()"

        /* GigE: Gain, Hue
        *  1394: Trigger Delay
        *  They will be treated as Floating in fact they are int
        */
        double dValue = 0;
        error = m_FeaturePtr_FloatSliderSpinBox->GetValue(dValue);

        if(VmbErrorSuccess == error)
        {
            m_EditBox_Float->setText( QString::number( dValue ));
        }
        else
        {
            emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (GetValue) "+ QString::number(dValue,'g',16)+" returned "+QString::number(error)+
                            " "+ Helper::mapReturnCodeToString(error)) ;
        }

        /* Slider */
        m_Slider_Float = new QwtSlider(Qt::Horizontal, m_FloatSliderEditWidget);
        m_Slider_Float->setGroove(true);
        m_Slider_Float->setTrough(false);
        m_Slider_Float->setScalePosition(static_cast<QwtSlider::ScalePosition>(0) /*::NoScale*/); // Unfortunately older g++ compiler does not support enum as namespace or class
        m_Slider_Float->setScaleEngine( new QwtLinearScaleEngine );
        m_Slider_Float->setHandleSize(QSize( 12, 18 ));
        m_Slider_Float->setFocusPolicy( Qt::StrongFocus);
        m_Slider_Float->installEventFilter(this);

        if (abs(m_dIncrement) > 0.0000001)
        {
            m_Slider_Float->setTotalSteps(ceil((m_dMaximum-m_dMinimum)/m_dIncrement));
        }

        m_Slider_Float->setScale( m_dMinimum, m_dMaximum );
        m_Slider_Float->setValue( dValue );

        m_HSliderEditLayout_Float2 = new QHBoxLayout();
        m_HSliderEditLayout_Float2->addWidget(m_Slider_Float);
        m_Slider_Float->setMinimumWidth(200);
        m_EditBox_Float->setMinimumWidth(60);
        m_EditBox_Float->setMaximumWidth(80);
        m_HSliderEditLayout_Float2->addWidget(m_EditBox_Float);
        m_EditBox_Float->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_HSpinSliderLayout_Float->addLayout(m_HSliderEditLayout_Float2);

        QPoint p = QCursor::pos();
        m_FloatSliderEditWidget->setFixedHeight(60);
        m_FloatSliderEditWidget->setWindowFlags( Qt::Tool | Qt::WindowCloseButtonHint );
        m_FloatSliderEditWidget->setWindowTitle(m_sCurrentSelectedFeature);
        AdjustOffscreenPosition(p, *m_FloatSliderEditWidget );
        m_FloatSliderEditWidget->move(p.x(), p.y());
        m_FloatSliderEditWidget->show();
        m_FloatSliderEditWidget->setFocus();

        /* As of Vimba >= 1.4, float features other than ExposureTime don't use spin box anymore */
        connect( m_Slider_Float, SIGNAL( valueChanged( double ) ), this, SLOT( onFloatSliderChanged( double ) ) );
        connect( m_EditBox_Float, SIGNAL(editingFinished()), this, SLOT(onFloatEditFinished()) );
        setCursor(Qt::PointingHandCursor);
    }
}

void ControllerTreeWindow::onFloatSliderReleased()
{
    onFloatSliderChanged(m_Slider_Float->value());
}

void ControllerTreeWindow::onFloatSliderChanged( double dValue )
{
    /* ignore it while pressing mouse and when slider still busy */
    if(m_bIsMousePressed || !m_bIsJobDone )
        return;

    m_bIsJobDone = false;

    if( dValue > (m_dMaximum * 0.995) )
        dValue = m_dMaximum;

    if(m_Slider_Float->hasFocus())
    {
        m_EditBox_Float->setText( QString::number(dValue));
        /* As of Vimba >= 1.4, float features other than ExposureTime don't use spin box anymore */
        onFloatEditFinished();
    }

    m_bIsJobDone = true;
}

void ControllerTreeWindow::onFloatEditFinished()
{
    double dValue = m_EditBox_Float->text().toDouble()*1.0000000000001; // allow some float imprecision
    setFloatingValue (dValue);
}

void ControllerTreeWindow::setFloatingValue ( const double &dValue )
{
    double dCurrentValue = dValue;
    VmbError_t error;

    /* restrict to the allowed values */
    if( m_dMaximum < dValue )
    {
        dCurrentValue = m_dMaximum;
    }
    else if( m_dMinimum > dValue )
    {
        dCurrentValue = m_dMinimum;
    }
    // removed handling for increment because of rounding errors
    // else if ( m_dIncrement != 0.0 )
    // {
    //     dCurrentValue = floor((dValue-m_dMinimum)/m_dIncrement)*m_dIncrement + m_dMinimum;
    // }
    error = m_FeaturePtr_FloatSliderSpinBox->SetValue(dCurrentValue);

    if(VmbErrorSuccess == error)
    {
        error = m_FeaturePtr_FloatSliderSpinBox->GetValue(dCurrentValue);

        if(VmbErrorSuccess == error)
        {
            const QString newValue = QString::number(dCurrentValue);
            if( m_EditBox_Float->text() != newValue )
            {
                m_EditBox_Float->setText( newValue );
            }

            if(!m_Slider_Float->hasFocus() || (!m_bIsGigE) || ( 0 == m_sCurrentSelectedFeature.compare("ColorTransformationValue")) )
            { /* Not GigE (except ColorTransformationValue), especially to handle Gamma feature. Make sure to set the current feature value back to slider */
                m_Slider_Float->setValue(dCurrentValue);
            }
        }

        /* reset all fps counters whenever shutter value has been changed */
        if( 0 == m_sFeature_FloatSliderSpinBox.compare("Exposure Time") ||
            0 == m_sFeature_FloatSliderSpinBox.compare("ExposureTimeAbs"))
        {
            emit resetFPS();
        }
    }
    else
    {
        emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (SetValue) "+ QString::number(dValue)+" returned "+QString::number(error)+
                      " "+ Helper::mapReturnCodeToString(error)) ;
    }
}

/* Logarithmic slider */
void ControllerTreeWindow::createLogarithmicSlider ( const QModelIndex &item )
{
    QString sFeature_FloatSliderSpinBox = getFeatureName(item);

    if (isFeatureWritable(sFeature_FloatSliderSpinBox))
    {
        m_sFeature_FloatSliderSpinBox = sFeature_FloatSliderSpinBox;

        m_LogSliderWidget  = new QWidget(this);
        m_HLogSliderLayout = new QHBoxLayout( m_LogSliderWidget );
        m_LogSliderWidget->setFixedHeight(80);
        m_LogSliderWidget->setMinimumWidth(400);

        /* LogSlider */
        m_LogSlider = new QwtSlider( Qt::Horizontal, m_LogSliderWidget);
        m_LogSlider->setGroove(true);
        m_LogSlider->setTrough(false);
        m_LogSlider->setScalePosition(static_cast<QwtSlider::ScalePosition>(2) /*::TrailingScale*/); // Unfortunately  oder g++ compiler does not support enum as namespace or class
        m_LogSlider->setScaleEngine(new QwtLogScaleEngine(10));
        m_LogSlider->setHandleSize(QSize(12, 18 ));
        m_LogSlider->setScale( m_dMinimum, m_dMaximum );

        m_LogSlider->setScaleMaxMinor( 10 );
        m_HLogSliderLayout->addWidget(m_LogSlider);
        m_LogSlider->setFocusPolicy( Qt::StrongFocus);
        connect( m_LogSlider, SIGNAL( valueChanged( double ) ), this, SLOT( setLogarithmicSliderValue( double ) ) );

        m_EditBox_Float = new QLineEdit(0);
        m_EditBox_Float->setObjectName("value");    //mark as the element of this dialog that contains the value, used in "updateWidget()"
        m_EditBox_Float->setMinimumWidth(60);
        m_EditBox_Float->setMaximumWidth(90);
        m_HLogSliderLayout->addWidget(m_EditBox_Float);
        m_EditBox_Float->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
        m_EditBox_Float->setFocusPolicy( Qt::StrongFocus);
        connect( m_EditBox_Float, SIGNAL(editingFinished()), this, SLOT(onLogarithmicFloatSpinBoxClick()) );

        double dCurrentValue = 0;
        m_FeaturePtr_FloatSliderSpinBox->GetValue(dCurrentValue);
        if (abs(m_dIncrement) > 0.0001)
        {
            dCurrentValue = floor((dCurrentValue-m_dMinimum)/m_dIncrement+0.00000001)*m_dIncrement + m_dMinimum;
            m_LogSlider->setTotalSteps(ceil((m_dMaximum-m_dMinimum)/m_dIncrement));
        }
        m_EditBox_Float->setText( QString::number( dCurrentValue ));
        m_LogSlider->setValue(dCurrentValue);
        m_LogSlider->installEventFilter(this);

        m_LogSliderWidget->setWindowTitle( sFeature_FloatSliderSpinBox + " (Min: " + QString::number(m_dMinimum,'g',6) + ", Max: " + QString::number(m_dMaximum,'g', 10) + ") "+ m_sCameraID );
        m_LogSliderWidget->setWindowFlags(Qt::WindowTitleHint);
        QPoint p = QCursor::pos();
        m_LogSliderWidget->setWindowFlags( Qt::Tool );
        AdjustOffscreenPosition(p, *m_LogSliderWidget );
        m_LogSliderWidget->move(p.x(), p.y());
        m_LogSliderWidget->show();

    }
}

void ControllerTreeWindow::onLogarithmicFloatSpinBoxClick()
{
    setLogarithmicFloatingValue ( m_EditBox_Float->text().toDouble() );
}

void ControllerTreeWindow::onLogarithmicSliderReleased()
{
    setLogarithmicSliderValue(m_LogSlider->value());
}

void ControllerTreeWindow::setLogarithmicSliderValue( double v )
{
    /* ignore it while pressing mouse and when slider still busy */
    if(m_bIsMousePressed || !m_bIsJobDone )
        return;

    m_bIsJobDone = false;

    double dValue = v;

    if( dValue > (m_dMaximum * 0.95) )
        dValue = m_dMaximum;

    if(m_LogSlider->hasFocus())
        setLogarithmicFloatingValue ( dValue );

    m_bIsJobDone = true;
}

void ControllerTreeWindow::setLogarithmicFloatingValue ( const double &dValue )
{
    VmbError_t Result;
    double dValueToWrite = dValue;
    if(dValue < m_dMinimum)
    {
        dValueToWrite = m_dMinimum;
    }
    else if(dValue > m_dMaximum)
    {
        dValueToWrite = m_dMaximum;
    }
    // removed adjustments for increments due to rounding problems
    // else if ( m_dIncrement != 0.0 )
    // {
    //     dValueToWrite = floor((dValueToWrite + 0.00001-m_dMinimum)/m_dIncrement)*m_dIncrement + m_dMinimum;
    // }

    Result = m_FeaturePtr_FloatSliderSpinBox->SetValue(dValueToWrite);
    if( Result != VmbErrorSuccess)
    {
        emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (SetValue) "+ " returned "+QString::number(Result)+
            " "+ Helper::mapReturnCodeToString(Result)) ;
        return;
    }
    double dCurrentValue = dValueToWrite;
    Result = m_FeaturePtr_FloatSliderSpinBox->GetValue(dCurrentValue);
    if( Result != VmbErrorSuccess)
    {
        emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (GetValue) "+ " returned "+QString::number(Result)+
            " "+ Helper::mapReturnCodeToString(Result)) ;
        return;
    }

    m_LogSlider->setValue(dCurrentValue);

    m_EditBox_Float->setText( QString::number( dCurrentValue ));
    m_LogSlider->setToolTip(QString::number(dCurrentValue,'g',16));

    emit resetFPS();
}

/* VmbFeatureDataBool */
void ControllerTreeWindow::createBooleanCheckBox ( const QModelIndex item )
{
    QString sFeature_CheckBox   = getFeatureName(item);

    if (isFeatureWritable(sFeature_CheckBox))
    {
        m_sCurrentSelectedFeature = m_sFeature_CheckBox = sFeature_CheckBox;

        m_FeaturePtr_CheckBox = getFeaturePtr(m_sFeature_CheckBox);
        if(NULL == m_FeaturePtr_CheckBox)
            return;

        resetControl();

        m_BooleanWidget  = new QWidget(this);
        m_HBooleanLayout = new QHBoxLayout (m_BooleanWidget);
        m_CheckBox_Bool = new QCheckBox (QString(""));
        m_CheckBox_Bool->setMaximumHeight(20);
        m_CheckBox_Bool->setObjectName("value");    //mark as the element of this dialog that contains the value, used in "updateWidget()"
        m_HBooleanLayout2 = new QHBoxLayout();
        m_HBooleanLayout2->addWidget(m_CheckBox_Bool);
        m_HBooleanLayout->addLayout(m_HBooleanLayout2);

        bool bValue = false;
        VmbError_t error = m_FeaturePtr_CheckBox->GetValue(bValue);
        if (VmbErrorSuccess == error)
        {
            m_CheckBox_Bool->setChecked(bValue);
        }
        else
        {
            m_CheckBox_Bool->setChecked(false);
            emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (GetValue) "+ " returned "+QString::number(error)+
                " "+ Helper::mapReturnCodeToString(error)) ;
        }

        QPoint p = QCursor::pos();
        m_BooleanWidget->setFixedHeight(60);
        m_CheckBox_Bool->setText(m_sCurrentSelectedFeature);

        m_BooleanWidget->setWindowFlags( Qt::Tool | Qt::WindowCloseButtonHint );
        m_BooleanWidget->setWindowTitle(m_sCurrentSelectedFeature);
        AdjustOffscreenPosition(p, *m_BooleanWidget );
        m_BooleanWidget->move(p.x(), p.y());
        m_BooleanWidget->show();

        connect( m_CheckBox_Bool, SIGNAL(clicked(bool)), this, SLOT(onBoolCheckBoxClick(bool)) );
        setCursor(Qt::PointingHandCursor);
    }
}

void ControllerTreeWindow::onBoolCheckBoxClick ( bool bValue )
{
    bool bCurrentValue = false;

    VmbError_t error = m_FeaturePtr_CheckBox->SetValue(bValue);
    if(VmbErrorSuccess == error)
    {
        error = m_FeaturePtr_CheckBox->GetValue(bCurrentValue);
        if(VmbErrorSuccess == error)
        {
            QString sValue = "false";
            (true == bCurrentValue) ? sValue = "true" : sValue = "false";
            m_CheckBox_Bool->setChecked(bCurrentValue);
        }
        else
        {
            emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (GetValue) "+ " returned "+QString::number(error)+
                          " "+ Helper::mapReturnCodeToString(error)) ;
        }
    }
    else
    {
        emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (SetValue) "+ QString::number(bValue) + " returned "+QString::number(error)+
                      " "+ Helper::mapReturnCodeToString(error)) ;
    }
}

/* VmbFeatureDataString */
void ControllerTreeWindow::createStringEditBox ( const QModelIndex item )
{
    QString sFeature_StringEditBox = getFeatureName(item);

    if (isFeatureWritable(sFeature_StringEditBox))
    {
        m_sCurrentSelectedFeature = m_sFeature_StringEditBox = sFeature_StringEditBox;

        m_FeaturePtr_StringEditBox = getFeaturePtr(m_sFeature_StringEditBox);
        if(NULL == m_FeaturePtr_StringEditBox)
            return;

        resetControl();

        m_EditWidget = new QWidget (this);
        m_HEditLayout = new QHBoxLayout(m_EditWidget);
        m_TextEdit_String = new QLineEdit();
        m_TextEdit_String->setObjectName("value");  //mark as the element of this dialog that contains the value, used in "updateWidget()"
        m_TextEdit_String->setMaximumHeight(20);
        m_HEditLayout2 = new QHBoxLayout();
        m_HEditLayout2->addWidget(m_TextEdit_String);
        m_HEditLayout->addLayout(m_HEditLayout2);

        std::string sValue;
        VmbError_t error = m_FeaturePtr_StringEditBox->GetValue(sValue);

        if(VmbErrorSuccess == error)
            m_TextEdit_String->setText(QString::fromUtf8(sValue.c_str()));
        else
            m_TextEdit_String->setText(QString::fromStdString(" "));

        QPoint p = QCursor::pos();
        m_EditWidget->setFixedHeight(60);
        m_EditWidget->setWindowFlags( Qt::Tool | Qt::WindowCloseButtonHint );
        m_EditWidget->setWindowTitle(m_sCurrentSelectedFeature);
        AdjustOffscreenPosition(p, *m_EditWidget );
        m_EditWidget->move(p.x(), p.y());
        m_EditWidget->show();

        connect( m_TextEdit_String, SIGNAL(returnPressed()), this, SLOT(onEditText()) );
        setCursor(Qt::PointingHandCursor);
    }
}

void ControllerTreeWindow::onEditText()
{
    QString sText = m_TextEdit_String->text();
    VmbError_t error = m_FeaturePtr_StringEditBox->SetValue((const char*)sText.toUtf8());

    if(VmbErrorSuccess == error)
    {
        std::string sCurrentValue;
        error = m_FeaturePtr_StringEditBox->GetValue(sCurrentValue);
        if(VmbErrorSuccess == error)
        {
            m_TextEdit_String->setText(QString::fromStdString(sCurrentValue));
        }
        else
        {
            m_TextEdit_String->setText("");
            emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (GetStringValue) "+ " returned "+QString::number(error)+
                          " "+ Helper::mapReturnCodeToString(error)) ;
        }
    }
    else
    {
        emit logging( "ERROR Feature: " + m_sCurrentSelectedFeature + " (SetStringValue) "+ sText  + " returned "+QString::number(error)+
            " "+ Helper::mapReturnCodeToString(error)) ;
    }
}

void ControllerTreeWindow::createLineEdit ( const QModelIndex &item )
{
    QString sFeature = getFeatureName(item);

    if (isFeatureWritable(sFeature))
    {
        m_sCurrentSelectedFeature = m_sFeature_StringLineEdit = sFeature;
        m_FeaturePtr_LineEdit = getFeaturePtr(m_sFeature_StringLineEdit);
        if(NULL == m_FeaturePtr_LineEdit)
            return;
        VmbFeatureDataType dt = VmbFeatureDataUnknown;
        VmbError_t error = m_FeaturePtr_LineEdit->GetDataType( dt );

        resetControl();

        QString sName = getFeatureNameFromMap(m_sCurrentSelectedFeature);
        std::string sRepresentation;
        m_FeaturePtr_LineEdit->GetRepresentation( sRepresentation );

        m_LineEditWidget = new QWidget(this);
        m_HLineEditLayout = new QHBoxLayout( m_LineEditWidget );

        /* add LineEdit that allows 0-255 */
        /* add Hex Label */
        m_LineEdit = new QLineEdit (m_LineEditWidget);
        m_LineEdit->setObjectName("value");     //mark as the element of this dialog that contains the value, used in "updateWidget()"

        if ( VmbFeatureDataInt == dt )
        {
            if( Helper::needsIPv4Format(sName, sRepresentation))
            {
                m_HexLabel = new QLabel ("", m_LineEditWidget);
                QRegExp rxIp("(([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])\\.){3}([01]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])");
                m_LineEdit->setValidator(new QRegExpValidator(rxIp, m_LineEdit));
                m_LineEdit->setMinimumWidth(20);
            }
            else if ( sRepresentation == "HexNumber" )
            {
                m_HexLabel = new QLabel (" 0x", m_LineEditWidget);
                QString s;

                QRegExp rxIp("([0-9a-fA-F]){1,8}");
                m_LineEdit->setValidator(new QRegExpValidator(rxIp, m_LineEdit));
                m_LineEdit->setMaxLength(8);
            }
            else // > sint32
            {
                m_HexLabel = new QLabel ("", m_LineEditWidget);
                QRegExp rxIp("-?[0-9]*");
                m_LineEdit->setValidator(new QRegExpValidator(rxIp, m_LineEdit));
                m_LineEdit->setMinimumWidth(20);
            }
        }
        else if( VmbFeatureDataFloat == dt )
        {
            m_HexLabel = new QLabel ("", m_LineEditWidget);
            m_LineEdit->setValidator(new QDoubleValidator(NULL));
            m_LineEdit->setMinimumWidth(20);
        }
        else
        {
            return;
        }

        m_HexLabel->setMaximumHeight(20);
        m_LineEdit->setMaximumHeight(20);

        m_HLineEditLayout2 = new QHBoxLayout( );
        m_HLineEditLayout2->addWidget(m_HexLabel);
        m_HLineEditLayout2->addWidget(m_LineEdit);
        m_HLineEditLayout->addLayout(m_HLineEditLayout2);

        if ( VmbFeatureDataInt == dt )
        {
            VmbInt64_t nValue64 = 0;
            error = m_FeaturePtr_LineEdit->GetValue(nValue64);
            if(VmbErrorSuccess == error)
            {
                /* Handling of Multicast IP Address */
                if( Helper::needsIPv4Format(sName, sRepresentation))
                {
                    m_LineEdit->insert(Helper::IPv4ToString(nValue64, true));
                }
                /* Handling of features like DeviceAccessRegisterValue or DeviceAccessRegisterAddress */
                else if ( sRepresentation == "HexNumber" )
                {
                    std::string sHexValue("");
                    QString sQHexValue;
                    error = m_FeaturePtr_LineEdit->GetValue(sHexValue);
                    if (!sHexValue.empty())
                    {
                        sQHexValue.fromStdString(sHexValue);
                    }
                    else
                    {
                        sQHexValue.sprintf("%x", nValue64);
                    }
                    m_LineEdit->insert(sQHexValue);
                }
                else
                {
                    m_LineEdit->insert(QString::number( nValue64 ));
                }
            }
            else
            {
                m_LineEdit->insert("Read Error");
            }
        }
        else if ( VmbFeatureDataFloat == dt )
        {
            double dValue = 0;
            error = m_FeaturePtr_LineEdit->GetValue( dValue );
            if( VmbErrorSuccess == error )
            {
                m_LineEdit->insert(QString::number( dValue ));
            }
            else
            {
                m_LineEdit->insert("Read Error");
            }
        }
        else
        {
            m_LineEdit->insert("Read Error");
        }

        QPoint p = QCursor::pos();
        m_LineEditWidget->setFixedHeight(60);
        m_LineEditWidget->setWindowFlags( Qt::Tool | Qt::WindowCloseButtonHint );
        m_LineEditWidget->setWindowTitle(m_sCurrentSelectedFeature);
        AdjustOffscreenPosition(p, *m_LineEditWidget );
        m_LineEditWidget->move(p.x(), p.y());
        m_LineEditWidget->show();

        bool success=false;
        success = connect(m_LineEdit, SIGNAL(returnPressed()), this, SLOT(onConfirmClick()) );
        setCursor(Qt::PointingHandCursor);
    }
}

void ControllerTreeWindow::onConfirmClick()
{
    m_HexLabel->clear();
    VmbError_t error;
    bool bOk;

    QString sName = getFeatureNameFromMap(m_sCurrentSelectedFeature);

    std::string sRepresentation;
    FeaturePtr pFeature = getFeaturePtr( m_sCurrentSelectedFeature );
    error = pFeature->GetRepresentation( sRepresentation );
    VmbFeatureDataType dt = VmbFeatureDataUnknown;
    pFeature->GetDataType( dt );

    if( VmbFeatureDataInt == dt )
    {
        qlonglong lValue = 0;

        if( Helper::needsIPv4Format(sName, sRepresentation))
        {
            m_HexLabel->setText("");
            /* read the value in byte */
            QString sIP = m_LineEdit->displayText();
            uint nSub0 = sIP.section('.', 0,0).toInt();
            uint nSub1 = sIP.section('.', 1,1).toInt();
            uint nSub2 = sIP.section('.', 2,2).toInt();
            uint nSub3 = sIP.section('.', -1,-1).toInt();

            /* convert to hex */
            QString sHexIP("");
            QString sHexSub("");

            /* set back to big endian, starting from nSub3 to nSub0 */
            sHexSub = sHexSub.setNum(nSub3,16);
            if(15 >= nSub3)
               sHexIP.append( "0"+ sHexSub );
            else
                sHexIP.append( sHexSub );

            sHexSub = sHexSub.setNum(nSub2,16);
            if(15 >= nSub2)
                sHexIP.append( "0"+ sHexSub );
            else
                sHexIP.append( sHexSub );

            sHexSub = sHexSub.setNum(nSub1,16);
            if(15 >= nSub1)
                sHexIP.append( "0"+ sHexSub );
            else
                sHexIP.append( sHexSub );

            sHexSub = sHexSub.setNum(nSub0,16);
            if(15 >= nSub0)
                sHexIP.append( "0"+ sHexSub );
            else
                sHexIP.append( sHexSub );

            lValue = sHexIP.toULongLong(&bOk, 16);

        }
        else if ( sRepresentation == "HexNumber")
        {
            m_HexLabel->setText(" 0x");
            /* convert hex to qlonglong */
            lValue = m_LineEdit->text().toLongLong(&bOk, 16);
        }
        else
        {
            m_HexLabel->setText("");
            lValue = m_LineEdit->text().toLongLong(&bOk);
            if ( !bOk && !lValue )
            {
                lValue = m_LineEdit->text().contains('-') ? std::numeric_limits<long long>::min() : std::numeric_limits<long long>::max();
            }
        }

        VmbInt64_t nCurrentValue64 = 0, nMin = 0, nMax = 0;
        VmbInt64_t nValue64 = (VmbInt64_t)lValue;
        error = pFeature->GetRange(nMin, nMax);
        if ( VmbErrorSuccess == error )
        {
            if ( nMin > nValue64 )
            {
                nValue64 = nMin;
            }
            if ( nMax < nValue64 )
            {
                nValue64 = nMax;
            }
            error = m_FeaturePtr_LineEdit->SetValue(nValue64);
        }

        if(VmbErrorSuccess == error)
        {
            /* if set is ok, read the value again and put it to GUI */
            error = m_FeaturePtr_LineEdit->GetValue(nCurrentValue64);
            if(VmbErrorSuccess != error)
            {
                if( Helper::needsIPv4Format(sName, sRepresentation))
                {
                    m_HexLabel->setText(tr("READ ERROR!!!"));
                }
                else if ( sRepresentation == "HexNumber" )
                {
                    m_HexLabel->setText(tr("READ ERROR!!!")+" 0x");
                }
                else
                {
                    m_HexLabel->setText(tr("READ ERROR!!!"));
                }

                return;
            }

            m_LineEdit->clear();

            if( Helper::needsIPv4Format(sName, sRepresentation))
            {
                m_LineEdit->insert(Helper::IPv4ToString(nCurrentValue64, true ) );
            }
            else if ( sRepresentation == "HexNumber" )
            {
                m_LineEdit->insert(QString::number(nCurrentValue64, 16));
            }
            else
            {
                m_LineEdit->insert(QString::number(nCurrentValue64));
            }
        }
        else
        {
            if( Helper::needsIPv4Format(sName, sRepresentation))
            {
                m_HexLabel->setText(tr("WRITE ERROR!!!"));
            }
            else if ( sRepresentation ==  "HexNumber" )
            {
                m_HexLabel->setText(tr("WRITE ERROR!!!")+" 0x");
            }
            else
            {
                m_HexLabel->setText(tr("WRITE ERROR!!!"));
            }
        }
    }
    else if ( VmbFeatureDataFloat == dt )
    {
        double dValue           = m_LineEdit->text().toDouble( &bOk );
        double dCurrentValue    = 0.0, dMin = 0.0, dMax = 0.0;

        if ( !bOk && !dValue )
        {
            m_HexLabel->setText(tr("ERROR!!!"));
            return;
        }
        error = pFeature->GetRange( dMin, dMax );
        if ( VmbErrorSuccess == error )
        {
            if ( dMin > dValue )
            {
                dValue = dMin;
            }
            if ( dMax < dValue )
            {
                dValue = dMax;
            }
            error = m_FeaturePtr_LineEdit->SetValue(dValue);
        }

        if(VmbErrorSuccess == error)
        {
            /* if set is ok, read the value again and put it to GUI */
            error = m_FeaturePtr_LineEdit->GetValue( dCurrentValue );
            if( VmbErrorSuccess != error )
            {
                {
                    m_HexLabel->setText(tr("READ ERROR!!!"));
                }

                return;
            }

            m_LineEdit->clear();

            m_LineEdit->insert( QString::number( dCurrentValue ));
        }
        else
        {
            m_HexLabel->setText(tr("WRITE ERROR!!!"));
        }
    }
}

bool ControllerTreeWindow::eventFilter(QObject *object, QEvent *event)
{
    /* set the slider focus back when loosing on scrolling, e.g scrolling right after changing spinbox */
    if (event->type() == QEvent::Wheel)
    {
        if (object == m_Slider_Int)
        {
            if(!m_Slider_Int->hasFocus())
                m_Slider_Int->setFocus();
        }

        if (object == m_Slider_Float)
        {
            if(!m_Slider_Float->hasFocus())
                m_Slider_Float->setFocus();
        }

        if (object == m_LogSlider)
        {
            if(!m_LogSlider->hasFocus())
                m_LogSlider->setFocus();
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

        if (object == m_LogSlider)
            onLogarithmicSliderReleased();
    }

    return false;
}

void ControllerTreeWindow::resetControl()
{
    /* delete button */
    if( NULL != m_ButtonWidget )
    {
        disconnect( m_CmdButton, SIGNAL(clicked()), this, SLOT(onCmdButtonClick()) );
        delete m_ButtonWidget;

        m_ButtonWidget   = NULL;
        m_CmdButton      = NULL;
        m_HButtonLayout  = NULL;
        m_HButtonLayout2 = NULL;
    }

    /* delete ComboBox */
    if( NULL != m_EnumComboBox )
    {
        disconnect( m_EnumComboBox, SIGNAL(activated(const QString &)), this, SLOT(onEnumComboBoxClick(const QString &)) );
        delete m_ComboWidget;

        m_ComboWidget   = NULL;
        m_EnumComboBox  = NULL;
        m_HComboLayout  = NULL;
        m_HComboLayout2 = NULL;
    }

    /* delete SpinBox+Slider Int */
    if( ( NULL != m_SpinBox_Int ) && ( NULL != m_Slider_Int ) &&
        ( NULL != m_HSpinSliderLayout_Int ) && ( NULL != m_IntSpinSliderWidget))
    {
        removeEventFilter(m_Slider_Int);
        disconnect( m_Slider_Int,  SIGNAL(valueChanged(int)), this, SLOT(onIntSliderChanged(int)) );
        disconnect( m_SpinBox_Int, SIGNAL(editingFinished()), this, SLOT(onIntSpinBoxClick()) );
        delete m_IntSpinSliderWidget;

        m_IntSpinSliderWidget    = NULL;
        m_SpinBox_Int            = NULL;
        m_Slider_Int             = NULL;
        m_HSpinSliderLayout_Int  = NULL;
        m_HSpinSliderLayout_Int2 = NULL;
    }

    /* delete SpinBox+Slider Float */
    if( ( NULL != m_EditBox_Float ) && ( NULL != m_Slider_Float ) &&
        ( NULL != m_HSpinSliderLayout_Float ) && ( NULL != m_FloatSliderEditWidget))
    {
        removeEventFilter(m_Slider_Float);
        disconnect( m_Slider_Float,  SIGNAL(valueChanged(double)), this, SLOT(onFloatSliderChanged(double)) );
        /* As of Vimba >= 1.4, float features other than ExposureTime don't use spin box anymore */
        disconnect( m_EditBox_Float, SIGNAL(editingFinished()), this, SLOT(onFloatEditFinished()) );
        delete m_FloatSliderEditWidget;

        m_FloatSliderEditWidget    = NULL;
        m_EditBox_Float            = NULL;
        m_Slider_Float             = NULL;
        m_HSpinSliderLayout_Float  = NULL;
        m_HSliderEditLayout_Float2 = NULL;
    }

    /* delete bool CheckBox  */
    if( NULL != m_CheckBox_Bool )
    {
        disconnect( m_CheckBox_Bool, SIGNAL(clicked(bool)), this, SLOT(onBoolCheckBoxClick(bool)) );
        delete m_BooleanWidget;

        m_BooleanWidget   = NULL;
        m_CheckBox_Bool   = NULL;
        m_HBooleanLayout  = NULL;
        m_HBooleanLayout2 = NULL;
    }

    /* delete TextEdit */
    if( NULL != m_TextEdit_String )
    {
        disconnect( m_TextEdit_String, SIGNAL(returnPressed()), this, SLOT(onEditText()) );
        delete m_EditWidget;

        m_EditWidget      = NULL;
        m_TextEdit_String = NULL;
        m_HEditLayout     = NULL;
        m_HEditLayout2    = NULL;
    }

    /* delete LineEdit for REG ADD or Multicast IP Address*/
    if( NULL != m_LineEdit )
    {
        disconnect( m_LineEdit, SIGNAL(returnPressed()), this, SLOT(onConfirmClick()) );
        delete m_LineEditWidget;

        m_LineEditWidget   = NULL;
        m_LineEdit         = NULL;
        m_HexLabel         = NULL;
        m_HLineEditLayout  = NULL;
        m_HLineEditLayout2 = NULL;
    }

    if ( NULL != m_HexWindow )
    {
        /* windows has been closed , just reset it */
        m_HexWindow = NULL;
        m_RawData.clear();
    }

    /* delete Logarithmic Slider */
    if( NULL != m_LogSliderWidget )
    {
        removeEventFilter(m_LogSlider);
        disconnect( m_LogSlider, SIGNAL( valueChanged( double ) ), this, SLOT( setLogarithmicSliderValue( double ) ) );
        delete m_LogSliderWidget;

        m_LogSliderWidget   = NULL;
        m_LogSlider         = NULL;
        m_HLogSliderLayout  = NULL;
        m_EditBox_Float     = NULL;
    }
}

void ControllerTreeWindow::showControl( const bool bIsShow )
{
    if ( m_IntSpinSliderWidget )
        m_IntSpinSliderWidget->setVisible( bIsShow );
    else if ( m_ButtonWidget )
        m_ButtonWidget->setVisible( bIsShow );
    else if ( m_ComboWidget )
        m_ComboWidget->setVisible( bIsShow );
    else if ( m_FloatSliderEditWidget )
        m_FloatSliderEditWidget->setVisible( bIsShow );
    else if ( m_BooleanWidget )
        m_BooleanWidget->setVisible( bIsShow );
    else if ( m_EditWidget )
        m_EditWidget->setVisible( bIsShow );
    else if ( m_LineEditWidget )
        m_LineEditWidget->setVisible( bIsShow );
    else if ( m_LogSliderWidget )
        m_LogSliderWidget->setVisible( bIsShow );
}

void ControllerTreeWindow::AdjustOffscreenPosition(QPoint &p, QWidget &parentWidget)
{
    Q_ASSERT( &parentWidget && "parentWidget is NULL reference" );

    /* out of screen? first identify the current screen */
    QDesktopWidget desktop;
    const QRect currentScreenGeometry = desktop.screenGeometry(desktop.screenNumber(p));

    parentWidget.adjustSize();

    /* target position: above and to the right of the targeted position p.x/p.y. */
    p.setY(p.y() - 10 - parentWidget.frameGeometry().height());


    /* if parts of the window would appear offscreen, move to the left */
    int horizontalCorrection = (p.x() - currentScreenGeometry.right()) + parentWidget.frameGeometry().width();
    if( horizontalCorrection > 0 )
        p.setX (p.x() - horizontalCorrection);

    /*  if vertical position would be offscreen, show below the current line */
    if( currentScreenGeometry.top() - p.y() > 0 )
        p.setY (p.y() + 24 + parentWidget.frameGeometry().height());
}

void ControllerTreeWindow::closeControls()
{
    resetControl();
}
