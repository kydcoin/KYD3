// Copyright (c) 2011-2014 The Bitcoin developers
// Copyright (c) 2014-2015 The Dash developers
// Copyright (c) 2015-2019 The PivX developers
// Copyright (c) 2018-2019 The KYD developers
// Distributed under the MIT/X11 software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include "overviewpage.h"
#include "ui_overviewpage.h"

#include "bitcoinunits.h"
#include "clientmodel.h"
#include "guiconstants.h"
#include "guiutil.h"
#include "init.h"
#include "obfuscation.h"
#include "obfuscationconfig.h"
#include "optionsmodel.h"
#include "transactionfilterproxy.h"
#include "transactionrecord.h"
#include "transactiontablemodel.h"
#include "walletmodel.h"
#include "newsitem.h"

#include <QtCore>
#include <QtNetwork>
#include <QAbstractItemDelegate>
#include <QPainter>
#include <QDebug>
#include <QSettings>
#include <QTimer>

#define DECORATION_SIZE 48
#define ICON_OFFSET 16
#define NUM_ITEMS 7

#define NEWS_URL "https://wallet-news.kydcoin.io/?call_custom_simple_rss=1"

extern CWallet* pwalletMain;

class TxViewDelegate : public QAbstractItemDelegate
{
    Q_OBJECT
public:
    TxViewDelegate() : QAbstractItemDelegate(), unit(BitcoinUnits::KYD)
    {
    }

    inline void paint(QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        painter->save();

        QIcon icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        QRect mainRect = option.rect;
        mainRect.moveLeft(ICON_OFFSET);
        QRect decorationRect(mainRect.topLeft(), QSize(DECORATION_SIZE, DECORATION_SIZE));
        int xspace = DECORATION_SIZE + 8;
        int ypad = 6;
        int halfheight = (mainRect.height() - 2 * ypad) / 2;
        QRect amountRect(mainRect.left() + xspace, mainRect.top() + ypad, mainRect.width() - xspace - ICON_OFFSET, halfheight);
        QRect addressRect(mainRect.left() + xspace, mainRect.top() + ypad + halfheight, mainRect.width() - xspace, halfheight);
        icon.paint(painter, decorationRect);

        QDateTime date = index.data(TransactionTableModel::DateRole).toDateTime();
        QString address = index.data(Qt::DisplayRole).toString();
        qint64 amount = index.data(TransactionTableModel::AmountRole).toLongLong();
        bool confirmed = index.data(TransactionTableModel::ConfirmedRole).toBool();

        // Check transaction status
        int nStatus = index.data(TransactionTableModel::StatusRole).toInt();
        bool fConflicted = false;
        if (nStatus == TransactionStatus::Conflicted || nStatus == TransactionStatus::NotAccepted) {
            fConflicted = true; // Most probably orphaned, but could have other reasons as well
        }
        bool fImmature = false;
        if (nStatus == TransactionStatus::Immature) {
            fImmature = true;
        }

        QVariant value = index.data(Qt::ForegroundRole);
        QColor foreground = COLOR_BLACK;
        if (value.canConvert<QBrush>()) {
            QBrush brush = qvariant_cast<QBrush>(value);
            foreground = brush.color();
        }

        painter->setPen(foreground);
        QRect boundingRect;
        painter->drawText(addressRect, Qt::AlignLeft | Qt::AlignVCenter, address, &boundingRect);

        if (index.data(TransactionTableModel::WatchonlyRole).toBool()) {
            QIcon iconWatchonly = qvariant_cast<QIcon>(index.data(TransactionTableModel::WatchonlyDecorationRole));
            QRect watchonlyRect(boundingRect.right() + 5, mainRect.top() + ypad + halfheight, 16, halfheight);
            iconWatchonly.paint(painter, watchonlyRect);
        }

        if(fConflicted) { // No need to check anything else for conflicted transactions
            foreground = COLOR_CONFLICTED;
        } else if (!confirmed || fImmature) {
            foreground = COLOR_UNCONFIRMED;
        } else if (amount < 0) {
            foreground = COLOR_NEGATIVE;
        } else {
            foreground = COLOR_BLACK;
        }
        painter->setPen(foreground);
        QString amountText = BitcoinUnits::formatWithUnit(unit, amount, true, BitcoinUnits::separatorAlways);
        if (!confirmed) {
            amountText = QString("[") + amountText + QString("]");
        }
        painter->drawText(amountRect, Qt::AlignRight | Qt::AlignVCenter, amountText);

        painter->setPen(COLOR_BLACK);
        painter->drawText(amountRect, Qt::AlignLeft | Qt::AlignVCenter, GUIUtil::dateTimeStr(date));

        painter->restore();
    }

    inline QSize sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
        return QSize(DECORATION_SIZE, DECORATION_SIZE);
    }

    int unit;
};

#include "overviewpage.moc"

OverviewPage::OverviewPage(QWidget* parent) : QWidget(parent),
                                              ui(new Ui::OverviewPage),
                                              clientModel(0),
                                              walletModel(0),
                                              currentBalance(-1),
                                              currentUnconfirmedBalance(-1),
                                              currentImmatureBalance(-1),
                                              currentWatchOnlyBalance(-1),
                                              currentWatchUnconfBalance(-1),
                                              currentWatchImmatureBalance(-1),
                                              txdelegate(new TxViewDelegate()),
                                              filter(0),
                                              currentReply(0)
{
    nDisplayUnit = 0; // just make sure it's not unitialized
    ui->setupUi(this);

    // Recent transactions
    ui->listTransactions->setItemDelegate(txdelegate);
    ui->listTransactions->setIconSize(QSize(DECORATION_SIZE, DECORATION_SIZE));
    ui->listTransactions->setMinimumHeight(NUM_ITEMS * (DECORATION_SIZE + 2));
    ui->listTransactions->setAttribute(Qt::WA_MacShowFocusRect, false);

    connect(ui->listTransactions, SIGNAL(clicked(QModelIndex)), this, SLOT(handleTransactionClicked(QModelIndex)));

    ui->listNews->setSortingEnabled(true);

    // init "out of sync" warning labels
    ui->labelWalletStatus->setText("(" + tr("out of sync") + ")");
    ui->labelTransactionsStatus->setText("(" + tr("out of sync") + ")");
    ui->labelNewsStatus->setText("(" + tr("out of sync") + ")");

    connect(&manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(newsFinished(QNetworkReply*)));

    SetLinks();

    // start with displaying the "out of sync" warnings
    showOutOfSyncWarning(true);

    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(updateNewsList()));
    timer->setInterval(10 * 1000); // after 10 seconds on the 1st cycle
    timer->setSingleShot(true);
    timer->start();
}

void OverviewPage::handleTransactionClicked(const QModelIndex& index)
{
    if (filter)
        emit transactionClicked(filter->mapToSource(index));
}

OverviewPage::~OverviewPage()
{
    delete ui;
}

void OverviewPage::setBalance(const CAmount& balance, const CAmount& unconfirmedBalance, const CAmount& immatureBalance,
                              const CAmount& watchOnlyBalance, const CAmount& watchUnconfBalance, const CAmount& watchImmatureBalance)
{
    currentBalance = balance;
    currentUnconfirmedBalance = unconfirmedBalance;
    currentImmatureBalance = immatureBalance;
    currentWatchOnlyBalance = watchOnlyBalance;
    currentWatchUnconfBalance = watchUnconfBalance;
    currentWatchImmatureBalance = watchImmatureBalance;

    CAmount nLockedBalance = 0;
    CAmount nWatchOnlyLockedBalance = 0;
    if (pwalletMain) {
        nLockedBalance = pwalletMain->GetLockedCoins();
        nWatchOnlyLockedBalance = pwalletMain->GetLockedWatchOnlyBalance();
    }

    // KYD Balance
    CAmount nTotalBalance = balance + unconfirmedBalance;
    CAmount kydAvailableBalance = balance - immatureBalance - nLockedBalance;

    // KYD Watch-Only Balance
    CAmount nTotalWatchBalance = watchOnlyBalance + watchUnconfBalance;
    CAmount nAvailableWatchBalance = watchOnlyBalance - watchImmatureBalance - nWatchOnlyLockedBalance;

    // KYD labels
    ui->labelBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, kydAvailableBalance, false, BitcoinUnits::separatorAlways));
    ui->labelUnconfirmed->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, unconfirmedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, immatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelLockedBalance->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nLockedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelTotal->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nTotalBalance, false, BitcoinUnits::separatorAlways));

    // Watchonly labels
    ui->labelWatchAvailable->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nAvailableWatchBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchPending->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchUnconfBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchImmature->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, watchImmatureBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchLocked->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nWatchOnlyLockedBalance, false, BitcoinUnits::separatorAlways));
    ui->labelWatchTotal->setText(BitcoinUnits::floorHtmlWithUnit(nDisplayUnit, nTotalWatchBalance, false, BitcoinUnits::separatorAlways));

    // Only show most balances if they are non-zero for the sake of simplicity
    QSettings settings;
    bool settingShowAllBalances = !settings.value("fHideZeroBalances").toBool();

    bool showWatchOnly = nTotalWatchBalance != 0;

    // KYD Available
    bool showKYDAvailable = settingShowAllBalances || kydAvailableBalance != nTotalBalance;
    bool showWatchOnlyKYDAvailable = showKYDAvailable || nAvailableWatchBalance != nTotalWatchBalance;
    ui->labelBalanceText->setVisible(showKYDAvailable || showWatchOnlyKYDAvailable);
    ui->labelBalance->setVisible(showKYDAvailable || showWatchOnlyKYDAvailable);
    ui->labelWatchAvailable->setVisible(showKYDAvailable && showWatchOnly);

    // KYD Pending
    bool showKYDPending = settingShowAllBalances || unconfirmedBalance != 0;
    bool showWatchOnlyKYDPending = showKYDPending || watchUnconfBalance != 0;
    ui->labelPendingText->setVisible(showKYDPending || showWatchOnlyKYDPending);
    ui->labelUnconfirmed->setVisible(showKYDPending || showWatchOnlyKYDPending);
    ui->labelWatchPending->setVisible(showKYDPending && showWatchOnly);

    // KYD Immature
    bool showImmature = settingShowAllBalances || immatureBalance != 0;
    bool showWatchOnlyImmature = showImmature || watchImmatureBalance != 0;
    ui->labelImmatureText->setVisible(showImmature || showWatchOnlyImmature);
    ui->labelImmature->setVisible(showImmature || showWatchOnlyImmature); // for symmetry reasons also show immature label when the watch-only one is shown
    ui->labelWatchImmature->setVisible(showWatchOnlyImmature && showWatchOnly); // show watch-only immature balance

    // KYD Locked
    bool showKYDLocked = settingShowAllBalances || nLockedBalance != 0;
    bool showWatchOnlyKYDLocked = showKYDLocked || nWatchOnlyLockedBalance != 0;
    ui->labelLockedBalanceText->setVisible(showKYDLocked || showWatchOnlyKYDLocked);
    ui->labelLockedBalance->setVisible(showKYDLocked || showWatchOnlyKYDLocked);
    ui->labelWatchLocked->setVisible(showKYDLocked && showWatchOnly);

    static int cachedTxLocks = 0;

    if (cachedTxLocks != nCompleteTXLocks) {
        cachedTxLocks = nCompleteTXLocks;
        ui->listTransactions->update();
    }
}

// show/hide watch-only labels
void OverviewPage::updateWatchOnlyLabels(bool showWatchOnly)
{
    ui->labelSpendable->setVisible(showWatchOnly);      // show spendable label (only when watch-only is active)
    ui->labelWatchonly->setVisible(showWatchOnly);      // show watch-only label
    ui->labelWatchAvailable->setVisible(showWatchOnly); // show watch-only available balance
    ui->labelWatchPending->setVisible(showWatchOnly);   // show watch-only pending balance
    ui->labelWatchLocked->setVisible(showWatchOnly);    // show watch-only total balance
    ui->labelWatchTotal->setVisible(showWatchOnly);     // show watch-only total balance

    if (!showWatchOnly) {
        ui->labelWatchImmature->hide();
    } else {
        ui->labelBalance->setIndent(20);
        ui->labelUnconfirmed->setIndent(20);
        ui->labelLockedBalance->setIndent(20);
        ui->labelImmature->setIndent(20);
        ui->labelTotal->setIndent(20);
    }
}

void OverviewPage::setClientModel(ClientModel* model)
{
    this->clientModel = model;
    if (model) {
        // Show warning if this is a prerelease version
        connect(model, SIGNAL(alertsChanged(QString)), this, SLOT(updateAlerts(QString)));
        updateAlerts(model->getStatusBarWarnings());
    }
}

void OverviewPage::setWalletModel(WalletModel* model)
{
    this->walletModel = model;
    if (model && model->getOptionsModel()) {
        // Set up transaction list
        filter = new TransactionFilterProxy();
        filter->setSourceModel(model->getTransactionTableModel());
        filter->setLimit(NUM_ITEMS);
        filter->setDynamicSortFilter(true);
        filter->setSortRole(Qt::EditRole);
        filter->setShowInactive(false);
        filter->sort(TransactionTableModel::Date, Qt::DescendingOrder);

        ui->listTransactions->setModel(filter);
        ui->listTransactions->setModelColumn(TransactionTableModel::ToAddress);

        // Keep up to date with wallet
        setBalance(model->getBalance(), model->getUnconfirmedBalance(), model->getImmatureBalance(),
                   model->getWatchBalance(), model->getWatchUnconfirmedBalance(), model->getWatchImmatureBalance());
        connect(model, SIGNAL(balanceChanged(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)), this,
                         SLOT(setBalance(CAmount, CAmount, CAmount, CAmount, CAmount, CAmount)));

        connect(model->getOptionsModel(), SIGNAL(displayUnitChanged(int)), this, SLOT(updateDisplayUnit()));
        connect(model->getOptionsModel(), SIGNAL(hideZeroBalancesChanged(bool)), this, SLOT(updateDisplayUnit()));

        updateWatchOnlyLabels(model->haveWatchOnly());
        connect(model, SIGNAL(notifyWatchonlyChanged(bool)), this, SLOT(updateWatchOnlyLabels(bool)));
    }

    // update the display unit, to not use the default ("KYD")
    updateDisplayUnit();
}

void OverviewPage::updateDisplayUnit()
{
    if (walletModel && walletModel->getOptionsModel()) {
        nDisplayUnit = walletModel->getOptionsModel()->getDisplayUnit();
        if (currentBalance != -1)
            setBalance(currentBalance, currentUnconfirmedBalance, currentImmatureBalance,
                currentWatchOnlyBalance, currentWatchUnconfBalance, currentWatchImmatureBalance);

        // Update txdelegate->unit with the current unit
        txdelegate->unit = nDisplayUnit;

        ui->listTransactions->update();
    }
}

void OverviewPage::updateAlerts(const QString& warnings)
{
    this->ui->labelAlerts->setVisible(!warnings.isEmpty());
    this->ui->labelAlerts->setText(warnings);
}

void OverviewPage::showOutOfSyncWarning(bool fShow)
{
    ui->labelWalletStatus->setVisible(fShow);
    ui->labelTransactionsStatus->setVisible(fShow);
}

void OverviewPage::SetLinks()
{
    ui->labelLinks1->setText("Website:");
    ui->labelLinks2->setText("Review:");
    ui->labelLinks3->setText("Platform:");
    ui->labelLinks4->setText("Block Explorer:");
    ui->labelLinks5->setText("Discord:");
    ui->labelLinks6->setText("Twitter:");
    ui->labelLinks7->setText("Github:");

    ui->labelLinksUrl1->setText("<a href=\"https://kydcoin.io\">https://kydcoin.io</a>");
    ui->labelLinksUrl2->setText("<a href=\"https://review.kydcoin.io\">https://review.kydcoin.io</a>");
    ui->labelLinksUrl3->setText("<a href=\"https://platform.kydcoin.io\">https://platform.kydcoin.io</a>");
    ui->labelLinksUrl4->setText("<a href=\"https://explorer.kydcoin.io\">https://explorer.kydcoin.io</a>");
    ui->labelLinksUrl5->setText("<a href=\"https://discord.gg/xjXpxqP\">https://discord.gg/xjXpxqP</a>");
    ui->labelLinksUrl6->setText("<a href=\"https://twitter.com/KYDcoin\">https://twitter.com/KYDcoin</a>");
    ui->labelLinksUrl7->setText("<a href=\"https://github.com/kydcoin\">https://github.com/kydcoin</a>");
}

void OverviewPage::updateNewsList()
{
    ui->labelNewsStatus->setVisible(true);

    xml.clear();

    QUrl url(NEWS_URL);
    newsGet(url);
}

void OverviewPage::newsGet(const QUrl &url)
{
    QNetworkRequest request(url);

    if (currentReply) {
        currentReply->disconnect(this);
        currentReply->deleteLater();
    }

    currentReply = manager.get(request);

    connect(currentReply, SIGNAL(readyRead()), this, SLOT(newsReadyRead()));
    connect(currentReply, SIGNAL(metaDataChanged()), this, SLOT(newsMetaDataChanged()));
    connect(currentReply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(newsError(QNetworkReply::NetworkError)));
}

void OverviewPage::newsMetaDataChanged()
{
    QUrl redirectionTarget = currentReply->attribute(QNetworkRequest::RedirectionTargetAttribute).toUrl();
    if (redirectionTarget.isValid()) {
        newsGet(redirectionTarget);
    }
}

void OverviewPage::newsReadyRead()
{
    int statusCode = currentReply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    if (statusCode >= 200 && statusCode < 300) {
        QByteArray data = currentReply->readAll();
        xml.addData(data);
    }
}

void OverviewPage::newsFinished(QNetworkReply *reply)
{
    Q_UNUSED(reply);

    parseXml();
    ui->labelNewsStatus->setVisible(false);

    // Timer Activation for the news refresh
    timer->setInterval(5 * 60 * 1000); // every 5 minutes
    timer->start();
}

void OverviewPage::parseXml()
{
    QString currentTag;
    QString linkString;
    QString titleString;
    QString pubDateString;
    QString authorString;
    QString descriptionString;

    bool insideItem = false;

    for(int i = 0; i < ui->listNews->count(); ++i)
    {
        delete ui->listNews->takeItem(i);
    }

    try {
        while (!xml.atEnd()) {
            xml.readNext();
            if (xml.isStartElement()) {
                currentTag = xml.name().toString();

                if (xml.name() == "item")
                {
                    insideItem = true;
                    titleString.clear();
                    pubDateString.clear();
                    authorString.clear();
                    descriptionString.clear();
                }
            } else if (xml.isEndElement()) {
                if (xml.name() == "item") {
                    if( !linkString.isEmpty() && !linkString.isNull()
                     && !titleString.isEmpty() && !titleString.isNull()
                     && !authorString.isEmpty() && !authorString.isNull()
                     && !pubDateString.isEmpty() && !pubDateString.isNull())
                    {
                        bool found = false;

                        QDateTime qdt = QDateTime::fromString(pubDateString,Qt::RFC2822Date);

                        for(int i = 0; i < ui->listNews->count(); ++i)
                        {
                            NewsItem * item = (NewsItem *)(ui->listNews->itemWidget(ui->listNews->item(i)));
                            if( item->pubDate == qdt )
                            {
                                found = true;
                                break;
                            }
                        }

                        if( !found )
                        {
                            NewsWidgetItem *widgetItem = new NewsWidgetItem(ui->listNews);
                            widgetItem->setData(Qt::UserRole,qdt);

                            ui->listNews->addItem(widgetItem);

                            NewsItem *newsItem = new NewsItem(this,qdt,linkString,titleString,authorString,descriptionString);

                            widgetItem->setSizeHint( newsItem->sizeHint() );

                            ui->listNews->setItemWidget( widgetItem, newsItem );
                        }
                    }

                    titleString.clear();
                    linkString.clear();
                    pubDateString.clear();
                    authorString.clear();
                    descriptionString.clear();

                    insideItem = false;
                }
            } else if (xml.isCharacters() && !xml.isWhitespace()) {
                if (insideItem) {
                    if (currentTag == "title")
                        titleString += xml.text().toString();
                    else if (currentTag == "link")
                        linkString += xml.text().toString();
                    else if (currentTag == "pubDate")
                        pubDateString += xml.text().toString();
                    else if (currentTag == "creator")
                        authorString += xml.text().toString();
                    else if (currentTag == "description")
                        descriptionString += xml.text().toString();
                }
            }
        }

        if (xml.error() && xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
            qWarning() << "XML ERROR:" << xml.lineNumber() << ": " << xml.errorString();
        }
    }

    catch(std::exception &e)
    {
        qWarning("std:exception %s",e.what());
    }

    catch(...)
    {
        qWarning("generic exception");
    }
}

void OverviewPage::newsError(QNetworkReply::NetworkError)
{
    qWarning("error retrieving RSS feed");

    currentReply->disconnect(this);
    currentReply->deleteLater();
    currentReply = 0;
}