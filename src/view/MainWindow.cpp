// src/view/MainWindow.cpp
#include "MainWindow.h"
#include "LeftSidebar.h"
#include "ManView.h"
#include "AiChatWidget.h"
#include "SettingsDialog.h"
#include "ResultViewDialog.h"
#include "SplitViewDialog.h"
#include "data/ManIndex.h"
#include "service/SearchService.h"
#include "service/ManService.h"
#include "service/ai/AiService.h"
#include "service/TranslationService.h"
#include "service/ExampleService.h"
#include "service/FavoriteService.h"
#include "service/HistoryService.h"
#include <DTitlebar>
#include <DDialog>
#include <DProgressBar>
#include <DLabel>
#include <QMenu>
#include <QAction>
#include <QSplitter>
#include <QShortcut>
#include <QIcon>
#include <QSettings>
#include <QEventLoop>
#include <QTimer>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QtConcurrent>

MainWindow::MainWindow(ManIndex* index, SearchService* searchSvc, ManService* manSvc,
                       AiService* aiSvc, TranslationService* trSvc,
                       FavoriteService* favSvc, HistoryService* histSvc,
                       QWidget* parent)
    : DMainWindow(parent), m_index(index), m_searchSvc(searchSvc), m_manSvc(manSvc),
      m_aiSvc(aiSvc), m_trSvc(trSvc), m_favSvc(favSvc), m_histSvc(histSvc)
{
    setWindowTitle("deepin man 手册");
    setWindowIcon(QIcon::fromTheme("deepin-iman", QIcon(":/assets/icons/deepin-iman.svg")));

    auto* titlebar = this->titlebar();
    if (titlebar) {
        titlebar->setIcon(QIcon::fromTheme("deepin-iman", QIcon(":/assets/icons/deepin-iman.svg")));
        titlebar->setTitle("deepin man 手册");

        m_btnPrev = new DIconButton(DStyle::SP_ArrowLeft, titlebar);
        m_btnNext = new DIconButton(DStyle::SP_ArrowRight, titlebar);
        m_btnPrev->setToolTip("后退");
        m_btnNext->setToolTip("前进");
        m_btnPrev->setEnabled(false);
        m_btnNext->setEnabled(false);
        titlebar->addWidget(m_btnPrev, Qt::AlignLeft);
        titlebar->addWidget(m_btnNext, Qt::AlignLeft);
        connect(m_btnPrev, &DIconButton::clicked, this, &MainWindow::onPrevPage);
        connect(m_btnNext, &DIconButton::clicked, this, &MainWindow::onNextPage);

        auto* menu = titlebar->menu();
        if (!menu) {
            menu = new QMenu(titlebar);
            titlebar->setMenu(menu);
        }
        auto* settingsAction = menu->addAction("AI 设置");
        connect(settingsAction, &QAction::triggered, this, &MainWindow::onOpenSettings);
        auto* favAction = menu->addAction("收藏当前页");
        connect(favAction, &QAction::triggered, this, &MainWindow::onToggleFavorite);
        auto* favListAction = menu->addAction("收藏列表");
        connect(favListAction, &QAction::triggered, this, &MainWindow::onShowFavorites);
        auto* histAction = menu->addAction("浏览历史");
        connect(histAction, &QAction::triggered, this, &MainWindow::onShowHistory);
        auto* refreshAction = menu->addAction("刷新索引");
        connect(refreshAction, &QAction::triggered, this, &MainWindow::onRefreshIndex);
        // DTitlebar 内置"关于"菜单项由 DApplication 统一提供，不再重复添加
    }

    auto* mainSplitter = new QSplitter(Qt::Horizontal, this);

    m_sidebar = new LeftSidebar;
    m_manView = new ManView;
    m_aiPanel = new AiChatWidget;

    mainSplitter->addWidget(m_sidebar);
    mainSplitter->addWidget(m_manView);
    mainSplitter->addWidget(m_aiPanel);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setStretchFactor(2, 0);
    mainSplitter->setSizes({240, 700, 360});

    setCentralWidget(mainSplitter);
    resize(1400, 850);

    QStringList ids = m_aiSvc->providerIds();
    QStringList names;
    for (const auto& id : ids) names << m_aiSvc->providerDisplayName(id);
    m_aiPanel->setProviderList(ids, names);
    m_aiPanel->setActiveProvider(m_aiSvc->activeProvider());
    updateAiModelInfo();

    connect(m_sidebar, &LeftSidebar::searchRequested, this, &MainWindow::onSearchRequested);
    connect(m_sidebar, &LeftSidebar::pageSelected, this, &MainWindow::onPageSelected);
    connect(m_manView, &ManView::crossReferenceClicked, this, &MainWindow::onCrossRefClicked);
    connect(m_manSvc, &ManService::pageRendered, this, &MainWindow::onPageRendered);

    connect(m_aiPanel, &AiChatWidget::providerChanged, m_aiSvc, &AiService::setActiveProvider);
    connect(m_aiPanel, &AiChatWidget::providerChanged, this, [this](const QString&) {
        updateAiModelInfo();
    });
    connect(m_aiPanel, &AiChatWidget::translateRequested, this, &MainWindow::onTranslateRequested);
    connect(m_aiPanel, &AiChatWidget::examplesRequested, this, &MainWindow::onExamplesRequested);
    connect(m_aiPanel, &AiChatWidget::questionAsked, this, &MainWindow::onQuestionAsked);
    connect(m_aiPanel, &AiChatWidget::parseCommandRequested, this, &MainWindow::onParseCommandRequested);

    auto* prevSc = new QShortcut(QKeySequence("Alt+Left"), this);
    auto* nextSc = new QShortcut(QKeySequence("Alt+Right"), this);
    connect(prevSc, &QShortcut::activated, this, &MainWindow::onPrevPage);
    connect(nextSc, &QShortcut::activated, this, &MainWindow::onNextPage);
}

void MainWindow::onSearchRequested(const QString& query, SearchService::SearchMode mode) {
    auto results = m_searchSvc->search(query, 50, mode);
    m_sidebar->setManPages(results);
}

void MainWindow::onPageSelected(const QString& name, int section) {
    openPage(name, section);
}

void MainWindow::onCrossRefClicked(const QString& name, int section) {
    openPage(name, section);
}

void MainWindow::openPage(const QString& name, int section) {
    auto pages = m_index->findByName(name);
    for (const auto& p : pages) {
        if (p.section == section) {
            if (m_currentPageId != -1) m_backStack.push(m_currentPageId);
            m_forwardStack.clear();
            m_currentPageId = p.id;
            m_manSvc->renderPageAsync(p.sourcePath);
            m_aiPanel->setCurrentPage(p);
            m_histSvc->recordVisit(p.id);
            setWindowTitle(QString("%1(%2) - deepin man 手册").arg(p.name).arg(p.section));
            updateNavButtons();
            return;
        }
    }
}

void MainWindow::onPageRendered(const QString& html) {
    m_manView->loadHtml(html);
}

void MainWindow::onPrevPage() {
    if (m_backStack.isEmpty()) return;
    m_forwardStack.push(m_currentPageId);
    m_currentPageId = m_backStack.pop();
    ManPage p = m_index->findById(m_currentPageId);
    if (!p.sourcePath.isEmpty()) {
        m_manSvc->renderPageAsync(p.sourcePath);
        m_aiPanel->setCurrentPage(p);
        setWindowTitle(QString("%1(%2) - deepin man 手册").arg(p.name).arg(p.section));
    }
    updateNavButtons();
}

void MainWindow::onNextPage() {
    if (m_forwardStack.isEmpty()) return;
    m_backStack.push(m_currentPageId);
    m_currentPageId = m_forwardStack.pop();
    ManPage p = m_index->findById(m_currentPageId);
    if (!p.sourcePath.isEmpty()) {
        m_manSvc->renderPageAsync(p.sourcePath);
        m_aiPanel->setCurrentPage(p);
        setWindowTitle(QString("%1(%2) - deepin man 手册").arg(p.name).arg(p.section));
    }
    updateNavButtons();
}

void MainWindow::updateNavButtons() {
    if (m_btnPrev) m_btnPrev->setEnabled(!m_backStack.isEmpty());
    if (m_btnNext) m_btnNext->setEnabled(!m_forwardStack.isEmpty());
}

void MainWindow::onOpenSettings() {
    SettingsDialog dlg(m_aiSvc->providerConfigs(), m_aiSvc->activeProvider(), this);
    if (dlg.exec() == DDialog::Accepted) {
        m_aiSvc->setProviderConfigs(dlg.configs());
        m_aiSvc->setActiveProvider(dlg.activeProvider());
        QStringList ids = m_aiSvc->providerIds();
        QStringList names;
        for (const auto& id : ids) names << m_aiSvc->providerDisplayName(id);
        m_aiPanel->setProviderList(ids, names);
        m_aiPanel->setActiveProvider(m_aiSvc->activeProvider());
        updateAiModelInfo();
    }
}

void MainWindow::onToggleFavorite() {
    if (m_currentPageId == -1) return;
    if (m_favSvc->isFavorite(m_currentPageId)) {
        m_favSvc->remove(m_currentPageId);
        m_aiPanel->appendMessage("系统", "已取消收藏");
    } else {
        m_favSvc->add(m_currentPageId, QString(), QString());
        m_aiPanel->appendMessage("系统", "已收藏当前页");
    }
}

void MainWindow::onTranslateRequested(const ManPage& page, const QString& targetLang) {
    m_aiPanel->appendMessage("系统", QString("正在翻译 %1 为%2...").arg(page.name).arg(targetLang));

    QString originalText;
    if (!page.sourcePath.isEmpty()) {
        QProcess p;
        p.start("mandoc", {"-Ttxt", page.sourcePath});
        if (p.waitForFinished(5000) && p.exitCode() == 0) {
            originalText = QString::fromUtf8(p.readAllStandardOutput());
        }
    }
    if (originalText.isEmpty()) originalText = page.title;

    m_trSvc->getTranslation(page, targetLang,
        [this, page, targetLang, originalText](const QString& result) {
            m_aiPanel->appendMessage("系统", QString("翻译完成，已弹出对照窗口。"));
            QString leftTitle = QString("原文：%1(%2)").arg(page.name).arg(page.section);
            QString rightTitle = QString("%1翻译").arg(targetLang);
            auto* dlg = new SplitViewDialog(leftTitle, originalText, rightTitle, result, this);
            dlg->exec();
            dlg->deleteLater();
        },
        [this](const QString& err) {
            m_aiPanel->appendMessage("错误", err);
        });
}

void MainWindow::onExamplesRequested(const ManPage& page) {
    m_aiPanel->appendMessage("系统", "正在生成 " + page.name + " 使用样例...");

    QString originalText;
    if (!page.sourcePath.isEmpty()) {
        QProcess p;
        p.start("mandoc", {"-Ttxt", page.sourcePath});
        if (p.waitForFinished(5000) && p.exitCode() == 0) {
            originalText = QString::fromUtf8(p.readAllStandardOutput());
        }
    }
    if (originalText.isEmpty()) originalText = page.title;

    auto* exampleSvc = new ExampleService(m_aiSvc, this);
    exampleSvc->generateExamples(page,
        [this, page, originalText](const QString& result) {
            m_aiPanel->appendMessage("系统", "样例生成完成，已弹出对照窗口。");
            QString leftTitle = QString("原文：%1(%2)").arg(page.name).arg(page.section);
            QString rightTitle = "AI 使用样例";
            auto* dlg = new SplitViewDialog(leftTitle, originalText, rightTitle, result, this);
            dlg->exec();
            dlg->deleteLater();
        },
        [this](const QString& err) {
            m_aiPanel->appendMessage("错误", err);
        });
}

void MainWindow::onQuestionAsked(const ManPage& page, const QString& question) {
    m_aiPanel->appendMessage("用户", question);
    m_aiSvc->askQuestion(page, question,
        [this](const AiChunk& chunk) {
            Q_UNUSED(chunk);
        },
        [this](const AiResult& result) {
            m_aiPanel->appendAiResult("AI", result.text, result.model);
        },
        [this](const QString& err) {
            m_aiPanel->appendMessage("错误", err);
        });
}

void MainWindow::onParseCommandRequested(const QString& cmdline) {
    QString cmd = m_aiSvc->parseCommandQuick(cmdline);
    if (cmd.isEmpty()) {
        m_aiPanel->appendMessage("错误", "无法解析命令");
        return;
    }
    auto pages = m_index->findByName(cmd);
    if (pages.isEmpty()) {
        m_aiPanel->appendMessage("错误", QString("未找到命令 %1 的 man 手册").arg(cmd));
        return;
    }
    m_aiPanel->appendMessage("系统", QString("解析命令: %1，已找到 %2(%3)，正在跳转...")
        .arg(cmd).arg(pages.first().name).arg(pages.first().section));
    openPage(pages.first().name, pages.first().section);
    m_aiPanel->appendMessage("系统", QString("已跳转到 %1(%2)").arg(pages.first().name).arg(pages.first().section));
}

void MainWindow::updateAiModelInfo() {
    auto* p = m_aiSvc->activeProviderPtr();
    if (p) {
        m_aiPanel->setProviderModelInfo(p->displayName(), p->model());
    } else {
        m_aiPanel->setProviderModelInfo("未配置", "");
    }
}

void MainWindow::onRefreshIndex() {
    DDialog progressDlg(this);
    progressDlg.setWindowTitle("正在刷新 man 手册索引...");
    auto* progressBar = new DProgressBar;
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    auto* label = new DLabel("正在扫描 man 手册，请稍候...");
    auto* layout = new QVBoxLayout;
    layout->addWidget(label);
    layout->addWidget(progressBar);
    auto* content = new QWidget;
    content->setLayout(layout);
    progressDlg.addContent(content);
    progressDlg.setFixedWidth(400);
    progressDlg.setModal(true);
    progressDlg.show();

    QEventLoop loop;
    connect(m_index, &ManIndex::scanProgress, this, [&](int cur, int total) {
        progressBar->setMaximum(total);
        progressBar->setValue(cur);
        QCoreApplication::processEvents();
    });
    connect(m_index, &ManIndex::scanFinished, this, [&](int) {
        progressDlg.accept();
        loop.quit();
    });

    QTimer::singleShot(0, this, [this]() {
        m_index->refreshManPages("/usr/share/man");
    });

    loop.exec();

    m_aiPanel->appendMessage("系统", QString("索引刷新完成，共 %1 页").arg(m_index->pageCount()));
}

void MainWindow::onShowFavorites() {
    auto items = m_favSvc->list();
    if (items.isEmpty()) {
        m_aiPanel->appendMessage("系统", "暂无收藏");
        return;
    }
    QStringList lines;
    for (const auto& item : items) {
        lines << QString("%1(%2)").arg(item.pageName).arg(item.pageSection);
    }
    auto* dlg = new ResultViewDialog("收藏列表", lines.join("\n"), this);
    dlg->exec();
    dlg->deleteLater();
}

void MainWindow::onShowHistory() {
    auto items = m_histSvc->recent(50);
    if (items.isEmpty()) {
        m_aiPanel->appendMessage("系统", "暂无浏览历史");
        return;
    }
    QStringList lines;
    for (const auto& item : items) {
        lines << QString("%1(%2) - %3")
                  .arg(item.pageName)
                  .arg(item.pageSection)
                  .arg(QDateTime::fromSecsSinceEpoch(item.visitedAt).toString("MM-dd HH:mm"));
    }
    auto* dlg = new ResultViewDialog("浏览历史", lines.join("\n"), this);
    dlg->exec();
    dlg->deleteLater();
}
