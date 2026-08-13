// src/view/SplitViewDialog.h
#pragma once
#include <DDialog>
#include <DTextEdit>
#include <DPushButton>
#include <DLabel>

DWIDGET_USE_NAMESPACE

class SplitViewDialog : public DDialog {
    Q_OBJECT
public:
    explicit SplitViewDialog(const QString& leftTitle, const QString& leftContent,
                            const QString& rightTitle, const QString& rightContent,
                            QWidget* parent = nullptr);

private:
    DTextEdit* m_leftView;
    DTextEdit* m_rightView;
};
