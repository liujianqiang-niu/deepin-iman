// src/view/ResultViewDialog.h
#pragma once
#include <DDialog>
#include <DTextEdit>
#include <DLabel>
#include <DPushButton>

DWIDGET_USE_NAMESPACE

class ResultViewDialog : public DDialog {
    Q_OBJECT
public:
    explicit ResultViewDialog(const QString& title, const QString& content, QWidget* parent = nullptr);

private:
    DTextEdit* m_textView;
};
