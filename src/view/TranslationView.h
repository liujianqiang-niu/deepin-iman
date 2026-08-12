// src/view/TranslationView.h
#pragma once
#include <DWidget>
#include <QTextBrowser>
#include <QSplitter>
#include <DPushButton>
#include <QComboBox>

DWIDGET_USE_NAMESPACE

class TranslationView : public DWidget {
    Q_OBJECT
public:
    explicit TranslationView(QWidget* parent = nullptr);

    void setEnglishHtml(const QString& html);
    void setChineseHtml(const QString& html);

private slots:
    void onModeChanged(int idx);

private:
    QComboBox* m_modeCombo;
    QSplitter* m_splitter;
    QTextBrowser* m_enView;
    QTextBrowser* m_zhView;
    QString m_enHtml;
    QString m_zhHtml;
};
