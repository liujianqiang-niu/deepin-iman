// src/view/DataManageDialog.cpp
#include "DataManageDialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <DLabel>

DataManageDialog::DataManageDialog(QWidget* parent) : DDialog(parent) {
    setTitle("数据管理");
    setFixedWidth(420);

    auto* widget = new QWidget;
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(10, 10, 10, 10);
    layout->setSpacing(8);

    auto* hint = new DLabel("勾选要清理的项目，点击确认后立即清除（不可恢复）：", widget);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    m_chkCache = new QCheckBox("清空翻译缓存", widget);
    m_chkHistory = new QCheckBox("清空浏览历史", widget);
    m_chkFavorites = new QCheckBox("清空收藏列表", widget);
    m_chkIndex = new QCheckBox("清空索引数据库（下次启动自动重建）", widget);
    layout->addWidget(m_chkCache);
    layout->addWidget(m_chkHistory);
    layout->addWidget(m_chkFavorites);
    layout->addWidget(m_chkIndex);

    addContent(widget);
    addButton("取消", false, DDialog::ButtonNormal);
    addButton("确认清理", true, DDialog::ButtonRecommend);
}

bool DataManageDialog::clearTranslationCache() const { return m_chkCache->isChecked(); }
bool DataManageDialog::clearHistory() const { return m_chkHistory->isChecked(); }
bool DataManageDialog::clearFavorites() const { return m_chkFavorites->isChecked(); }
bool DataManageDialog::clearIndex() const { return m_chkIndex->isChecked(); }
