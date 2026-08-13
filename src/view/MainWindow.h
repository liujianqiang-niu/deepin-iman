// src/view/MainWindow.h
#pragma once
#include <DMainWindow>
#include <DIconButton>
#include <QStack>
#include <QTextBrowser>
#include <DPushButton>
#include "data/ManIndex.h"

DWIDGET_USE_NAMESPACE

class LeftSidebar;
class ManView;
class AiChatWidget;
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
    void onPageSelected(const QString& name, int section);
    void onCrossRefClicked(const QString& name, int section);
    void onPageRendered(const QString& html);
    void onPrevPage();
    void onNextPage();
    void onOpenSettings();
    void onToggleFavorite();
    void onRefreshIndex();
    void onShowFavorites();
    void onShowHistory();
    void onTranslateRequested(const ManPage& page, const QString& targetLang);
    void onExamplesRequested(const ManPage& page);
    void onQuestionAsked(const ManPage& page, const QString& question);
    void onParseCommandRequested(const QString& cmdline);

private:
    LeftSidebar* m_sidebar;
    ManView* m_manView;
    QTextBrowser* m_resultView;
    DPushButton* m_resultCloseBtn;
    AiChatWidget* m_aiPanel;

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
    void showResultPanel(const QString& title, const QString& content);
    void hideResultPanel();
    static QString markdownToHtml(const QString& md);
};
