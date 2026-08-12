// src/view/MainWindow.h
#pragma once
#include <DMainWindow>
#include <QStack>

DWIDGET_USE_NAMESPACE

class LeftSidebar;
class ManView;
class ManIndex;
class SearchService;
class ManService;

class MainWindow : public DMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(ManIndex* index, SearchService* searchSvc, ManService* manSvc, QWidget* parent = nullptr);

private slots:
    void onSearchRequested(const QString& query);
    void onPageSelected(const QString& name, int section);
    void onCrossRefClicked(const QString& name, int section);
    void onPageRendered(const QString& html);
    void onPrevPage();
    void onNextPage();

private:
    LeftSidebar* m_sidebar;
    ManView* m_manView;
    ManIndex* m_index;
    SearchService* m_searchSvc;
    ManService* m_manSvc;

    QStack<int> m_backStack;
    QStack<int> m_forwardStack;
    int m_currentPageId = -1;

    void openPage(const QString& name, int section);
};
