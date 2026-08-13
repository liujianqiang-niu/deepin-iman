// src/view/EditorPanel.h
#pragma once
#include <DWidget>
#include <DIconButton>
#include <DLabel>
#include <QString>

DWIDGET_USE_NAMESPACE

class EditorPanel : public DWidget {
    Q_OBJECT
public:
    explicit EditorPanel(QWidget* parent = nullptr);

    void setTitle(const QString& title);
    QString title() const;
    void setContent(QWidget* w);
    QWidget* contentWidget() const;

signals:
    void closed();

private:
    DLabel* m_titleLabel;
    DIconButton* m_closeBtn;
    QWidget* m_contentHost;
};
