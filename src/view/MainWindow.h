// src/view/MainWindow.h
#pragma once
#include <DMainWindow>
#include <DIconButton>
#include <QStack>
#include "data/ManIndex.h"

DWIDGET_USE_NAMESPACE

class LeftSidebar;
class ManView;
class AiChatWidget;
class TerminalPanel;
class ManIndex;
class SearchService;
class ManService;
class AiService;
class TranslationService;
class ExampleService;
class FavoriteService;
class HistoryService;
class FavoriteDb;
class HistoryDb;
class TranslationCache;

class MainWindow : public DMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(ManIndex* index, SearchService* searchSvc, ManService* manSvc,
                        AiService* aiSvc, TranslationService* trSvc,
                        FavoriteService* favSvc, HistoryService* histSvc,
                        QWidget* parent = nullptr);

private slots:
    void onSearchRequested(const QString& query);
    void onPageSelected(const QString& name, int section);
    void onCrossRefClicked(const QString& name, int section);
    void onPageRendered(const QString& html);
    void onPrevPage();
    void onNextPage();
    void onOpenSettings();
    void onToggleTerminal();
    void onToggleFavorite();
    void onRefreshIndex();
    void onExportMarkdown();
    void onTranslateRequested(const ManPage& page, const QString& targetLang);
    void onExamplesRequested(const ManPage& page);
    void onQuestionAsked(const ManPage& page, const QString& question);
    void onParseCommandRequested(const QString& cmdline);

private:
    LeftSidebar* m_sidebar;
    ManView* m_manView;
    AiChatWidget* m_aiPanel;
    TerminalPanel* m_terminal;

    ManIndex* m_index;
    SearchService* m_searchSvc;
    ManService* m_manSvc;
    AiService* m_aiSvc;
    TranslationService* m_trSvc;
    FavoriteService* m_favSvc;
    HistoryService* m_histSvc;

    DIconButton* m_btnPrev;
    DIconButton* m_btnNext;

    QStack<int> m_backStack;
    QStack<int> m_forwardStack;
    int m_currentPageId = -1;

    void openPage(const QString& name, int section);
    void updateNavButtons();
    void updateAiModelInfo();
    void refreshSidebarLists();
};
