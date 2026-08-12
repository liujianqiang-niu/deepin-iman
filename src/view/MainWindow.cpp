// src/view/MainWindow.cpp
#include "MainWindow.h"
#include "LeftSidebar.h"
#include "ManView.h"
#include "data/ManIndex.h"
#include "service/SearchService.h"
#include "service/ManService.h"
#include <DTitlebar>
#include <QSplitter>
#include <QShortcut>
#include <QAction>

MainWindow::MainWindow(ManIndex* index, SearchService* searchSvc, ManService* manSvc, QWidget* parent)
    : DMainWindow(parent), m_index(index), m_searchSvc(searchSvc), m_manSvc(manSvc)
{
    setWindowTitle(tr("deepin iman"));
    setWindowIcon(QIcon::fromTheme("deepin-iman", QIcon(":/icons/deepin-iman.svg")));

    auto* splitter = new QSplitter(Qt::Horizontal, this);

    m_sidebar = new LeftSidebar;
    m_manView = new ManView;

    splitter->addWidget(m_sidebar);
    splitter->addWidget(m_manView);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes({240, 800});

    setCentralWidget(splitter);
    resize(1200, 800);

    auto* titlebar = this->titlebar();
    if (titlebar) {
        m_btnPrev = new DIconButton(DStyle::SP_ArrowLeft, titlebar);
        m_btnNext = new DIconButton(DStyle::SP_ArrowRight, titlebar);
        m_btnPrev->setToolTip(tr("Back"));
        m_btnNext->setToolTip(tr("Forward"));
        m_btnPrev->setEnabled(false);
        m_btnNext->setEnabled(false);
        titlebar->addWidget(m_btnPrev, Qt::AlignLeft);
        titlebar->addWidget(m_btnNext, Qt::AlignLeft);
        connect(m_btnPrev, &DIconButton::clicked, this, &MainWindow::onPrevPage);
        connect(m_btnNext, &DIconButton::clicked, this, &MainWindow::onNextPage);
    }

    connect(m_sidebar, &LeftSidebar::searchRequested, this, &MainWindow::onSearchRequested);
    connect(m_sidebar, &LeftSidebar::pageSelected, this, &MainWindow::onPageSelected);
    connect(m_manView, &ManView::crossReferenceClicked, this, &MainWindow::onCrossRefClicked);
    connect(m_manSvc, &ManService::pageRendered, this, &MainWindow::onPageRendered);

    auto* prevSc = new QShortcut(QKeySequence("Alt+Left"), this);
    auto* nextSc = new QShortcut(QKeySequence("Alt+Right"), this);
    connect(prevSc, &QShortcut::activated, this, &MainWindow::onPrevPage);
    connect(nextSc, &QShortcut::activated, this, &MainWindow::onNextPage);
}

void MainWindow::onSearchRequested(const QString& query) {
    auto results = m_searchSvc->search(query, 50);
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
            setWindowTitle(tr("%1(%2) - deepin iman").arg(p.name).arg(p.section));
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
        setWindowTitle(tr("%1(%2) - deepin iman").arg(p.name).arg(p.section));
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
        setWindowTitle(tr("%1(%2) - deepin iman").arg(p.name).arg(p.section));
    }
    updateNavButtons();
}

void MainWindow::updateNavButtons() {
    if (m_btnPrev) m_btnPrev->setEnabled(!m_backStack.isEmpty());
    if (m_btnNext) m_btnNext->setEnabled(!m_forwardStack.isEmpty());
}
