// src/view/MainWindow.cpp
#include "MainWindow.h"
#include "LeftSidebar.h"
#include "ManView.h"
#include "AiChatWidget.h"
#include "SettingsDialog.h"
#include "PageListDialog.h"
#include "DataManageDialog.h"
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
#include <QProcess>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QScrollBar>
#include <QEvent>

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
        auto* dataAction = menu->addAction("数据管理");
        connect(dataAction, &QAction::triggered, this, &MainWindow::onDataManage);
        // DTitlebar 内置"关于"菜单项由 DApplication 统一提供，不再重复添加
    }

    auto* mainSplitter = new QSplitter(Qt::Horizontal, this);
    m_mainSplitter = mainSplitter;

    m_sidebar = new LeftSidebar;
    m_manView = new ManView;

    m_manPanel = new EditorPanel;
    m_manPanel->setTitle("原文");
    m_manPanel->setContent(m_manView);

    m_trPanel = new EditorPanel;
    m_trView = new QTextBrowser;
    m_trView->setOpenExternalLinks(true);
    m_trPanel->setTitle("翻译");
    m_trPanel->setContent(m_trView);
    m_trPanel->setVisible(false);

    m_aiPanel = new AiChatWidget;

    mainSplitter->addWidget(m_sidebar);
    mainSplitter->addWidget(m_manPanel);
    mainSplitter->addWidget(m_trPanel);
    mainSplitter->addWidget(m_aiPanel);
    mainSplitter->setStretchFactor(0, 0);
    mainSplitter->setStretchFactor(1, 1);
    mainSplitter->setStretchFactor(2, 1);
    mainSplitter->setStretchFactor(3, 0);
    mainSplitter->setSizes({240, 600, 600, 360});

    connect(m_manPanel, &EditorPanel::closed, this, &MainWindow::onManPanelClosed);
    connect(m_trPanel, &EditorPanel::closed, this, &MainWindow::onTrPanelClosed);
    connect(m_manPanel, &EditorPanel::detachRequested, this, &MainWindow::onDetachManPanel);
    connect(m_trPanel, &EditorPanel::detachRequested, this, &MainWindow::onDetachTrPanel);

    setCentralWidget(mainSplitter);
    resize(1400, 850);

    QStringList ids = m_aiSvc->providerIds();
    QStringList names;
    for (const auto& id : ids) names << m_aiSvc->providerDisplayName(id);
    m_aiPanel->setProviderList(ids, names);
    m_aiPanel->setActiveProvider(m_aiSvc->activeProvider());
    updateAiModelInfo();

    connect(m_sidebar, &LeftSidebar::searchRequested, this,
        [this](const QString& query, bool caseSensitive, bool wholeWord) {
            auto results = m_searchSvc->search(query, 50, caseSensitive, wholeWord);
            m_sidebar->setManPages(results);
        });
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

    auto* prevSc = new QShortcut(QKeySequence("Alt+Left"), this);
    auto* nextSc = new QShortcut(QKeySequence("Alt+Right"), this);
    connect(prevSc, &QShortcut::activated, this, &MainWindow::onPrevPage);
    connect(nextSc, &QShortcut::activated, this, &MainWindow::onNextPage);
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
            m_histSvc->recordVisit(p.id, p.name, p.section);
            setWindowTitle(QString("%1(%2) - deepin man 手册").arg(p.name).arg(p.section));
            updateNavButtons();
            return;
        }
    }
}

void MainWindow::onPageRendered(const QString& html) {
    if (!m_manPanel->isVisible()) m_manPanel->setVisible(true);
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
        ManPage p = m_index->findById(m_currentPageId);
        m_favSvc->add(m_currentPageId, p.name, p.section, QString(), QString());
        m_aiPanel->appendMessage("系统", QString("已收藏 %1(%2)").arg(p.name).arg(p.section));
    }
}

void MainWindow::onTranslateRequested(const ManPage& page) {
    m_aiPanel->appendMessage("系统", QString("正在翻译 %1 为中文...").arg(page.name));

    m_trSvc->getTranslation(page,
        [this, page](const QString& result, bool isHtml) {
            m_aiPanel->appendMessage("系统", QString("翻译完成，右侧面板已显示。"));
            showResultPanel(QString("翻译：%1(%2) → 中文").arg(page.name).arg(page.section), result, isHtml);
        },
        [this](const QString& err) {
            m_aiPanel->appendMessage("错误", err);
            hideResultPanel();
        });
}

void MainWindow::onExamplesRequested(const ManPage& page) {
    m_aiPanel->appendMessage("系统", "正在生成 " + page.name + " 使用样例...");

    auto* exampleSvc = new ExampleService(m_aiSvc, this);
    exampleSvc->generateExamples(page,
        [this, page](const QString& result) {
            m_aiPanel->appendMessage("系统", "样例生成完成，右侧面板已显示。");
            showResultPanel(QString("样例：%1(%2)").arg(page.name).arg(page.section), result, false);
        },
        [this](const QString& err) {
            m_aiPanel->appendMessage("错误", err);
            hideResultPanel();
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

void MainWindow::updateAiModelInfo() {
    auto* p = m_aiSvc->activeProviderPtr();
    if (p) {
        m_aiPanel->setProviderModelInfo(p->displayName(), p->model());
    } else {
        m_aiPanel->setProviderModelInfo("未配置", "");
    }
}

void MainWindow::showResultPanel(const QString& title, const QString& content, bool isHtml) {
    m_trPanel->setTitle(title);
    if (isHtml) {
        m_trView->setHtml(content);
    } else {
        m_trView->setMarkdown(content);
    }
    m_trPanel->setVisible(true);
}

void MainWindow::hideResultPanel() {
    m_trPanel->setVisible(false);
}

void MainWindow::onManPanelClosed() {
    m_manPanel->setVisible(false);
}

void MainWindow::onTrPanelClosed() {
    m_trPanel->setVisible(false);
}

void MainWindow::onDetachManPanel() { detachPanel(m_manPanel); }
void MainWindow::onDetachTrPanel() { detachPanel(m_trPanel); }

void MainWindow::detachPanel(EditorPanel* panel) {
    if (m_detachedWindows.contains(panel)) return;

    int index = m_mainSplitter->indexOf(panel);
    if (index < 0) return;

    panel->setParent(nullptr);
    panel->setDetached(true);

    auto* win = new DMainWindow;
    win->setAttribute(Qt::WA_DeleteOnClose);
    win->setWindowTitle(panel->title());
    win->resize(700, 600);
    win->installEventFilter(this);
    win->setCentralWidget(panel);
    panel->setVisible(true);

    m_detachedWindows[panel] = win;
    win->show();
}

void MainWindow::reattachPanel(EditorPanel* panel) {
    if (!m_detachedWindows.contains(panel)) return;

    auto* win = m_detachedWindows.take(panel);

    panel->setParent(this);
    panel->setDetached(false);

    if (panel == m_manPanel) {
        m_mainSplitter->insertWidget(1, panel);
    } else if (panel == m_trPanel) {
        m_mainSplitter->insertWidget(2, panel);
    }
    panel->setVisible(true);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::Close) {
        for (auto it = m_detachedWindows.begin(); it != m_detachedWindows.end(); ++it) {
            if (it.value() == obj) {
                reattachPanel(it.key());
                break;
            }
        }
    }
    return DMainWindow::eventFilter(obj, event);
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

    connect(m_index, &ManIndex::scanProgress, this, [&](int cur, int total) {
        progressBar->setMaximum(total);
        progressBar->setValue(cur);
        QCoreApplication::processEvents();
    });

    m_index->refreshManPages("/usr/share/man");
    progressDlg.accept();

    m_aiPanel->appendMessage("系统", QString("索引刷新完成，共 %1 页").arg(m_index->pageCount()));
}

void MainWindow::onShowFavorites() {
    auto items = m_favSvc->list();
    if (items.isEmpty()) {
        m_aiPanel->appendMessage("系统", "暂无收藏");
        return;
    }
    auto* dlg = new PageListDialog("收藏列表 — 双击跳转，勾选删除", this);
    dlg->setFavorites(items);
    connect(dlg, &PageListDialog::pageSelected, this, [this](const QString& name, int section) {
        openPage(name, section);
    });
    connect(dlg, &PageListDialog::favoritesDeleted, this, [this](const QList<int>& ids) {
        m_favSvc->deleteByIds(ids);
        m_aiPanel->appendMessage("系统", QString("已删除 %1 个收藏").arg(ids.size()));
    });
    dlg->exec();
    dlg->deleteLater();
}

void MainWindow::onShowHistory() {
    auto items = m_histSvc->recent(50);
    if (items.isEmpty()) {
        m_aiPanel->appendMessage("系统", "暂无浏览历史");
        return;
    }
    auto* dlg = new PageListDialog("浏览历史 — 双击跳转，勾选删除", this);
    dlg->setHistory(items);
    connect(dlg, &PageListDialog::pageSelected, this, [this](const QString& name, int section) {
        openPage(name, section);
    });
    connect(dlg, &PageListDialog::historyDeleted, this, [this](const QList<int>& ids) {
        m_histSvc->deleteByIds(ids);
        m_aiPanel->appendMessage("系统", QString("已删除 %1 条历史").arg(ids.size()));
    });
    dlg->exec();
    dlg->deleteLater();
}

void MainWindow::onDataManage() {
    DataManageDialog dlg(this);
    if (dlg.exec() != DDialog::Accepted) return;

    int totalSteps = 0;
    if (dlg.clearTranslationCache()) ++totalSteps;
    if (dlg.clearHistory()) ++totalSteps;
    if (dlg.clearFavorites()) ++totalSteps;
    if (dlg.clearIndex()) ++totalSteps;
    if (totalSteps == 0) {
        m_aiPanel->appendMessage("系统", "未选择任何清理项");
        return;
    }

    DDialog progressDlg(this);
    progressDlg.setWindowTitle("正在清理...");
    progressDlg.setFixedWidth(400);
    auto* progressBar = new DProgressBar;
    progressBar->setRange(0, totalSteps);
    progressBar->setValue(0);
    auto* layout = new QVBoxLayout;
    layout->addWidget(progressBar);
    auto* content = new QWidget;
    content->setLayout(layout);
    progressDlg.addContent(content);
    progressDlg.setModal(true);
    progressDlg.show();

    int step = 0;
    QStringList done;
    auto stepDone = [&](const QString& name) {
        done << name;
        progressBar->setValue(++step);
        QCoreApplication::processEvents();
    };

    if (dlg.clearTranslationCache()) {
        m_trSvc->clearCache();
        stepDone("翻译缓存");
    }
    if (dlg.clearHistory()) {
        m_histSvc->clearAll();
        stepDone("浏览历史");
    }
    if (dlg.clearFavorites()) {
        m_favSvc->clearAll();
        stepDone("收藏列表");
    }
    if (dlg.clearIndex()) {
        m_index->clearAll();
        stepDone("索引数据库");
    }

    progressDlg.accept();
    m_aiPanel->appendMessage("系统", QString("已清理：%1").arg(done.join("、")));
    if (dlg.clearIndex()) {
        m_aiPanel->appendMessage("系统", "索引已清空，下次启动时将自动重新扫描");
    }
}
