// src/view/DataManageDialog.h
#pragma once
#include <DDialog>
#include <QCheckBox>

DWIDGET_USE_NAMESPACE

class DataManageDialog : public DDialog {
    Q_OBJECT
public:
    explicit DataManageDialog(QWidget* parent = nullptr);

    bool clearTranslationCache() const;
    bool clearHistory() const;
    bool clearFavorites() const;
    bool clearIndex() const;

private:
    QCheckBox* m_chkCache;
    QCheckBox* m_chkHistory;
    QCheckBox* m_chkFavorites;
    QCheckBox* m_chkIndex;
};
