/****************************************************************************
** CharSet Tool
** Copyright Fougue Ltd. (15 Apr. 2014)
** contact@fougsys.fr
**
** This software is a computer program whose purpose is to analyse and convert
** the encoding of text files.
**
** This software is governed by the CeCILL-B license under French law and
** abiding by the rules of distribution of free software.  You can  use,
** modify and/ or redistribute the software under the terms of the CeCILL-B
** license as circulated by CEA, CNRS and INRIA at the following URL
** "http://www.cecill.info/licences/Licence_CeCILL-B_V1-en.html".
****************************************************************************/

#include "select_charset_dialog.h"
#include "ui_select_charset_dialog.h"

#include <algorithm>
#include <QtCore/QMap>
#include <QtCore/QSettings>
#include <QtCore/QStringList>
#include <QtCore/QTextCodec>
#include <QtCore/QSortFilterProxyModel>
#include <QtGui/QStandardItemModel>
#include <QtWidgets/QMessageBox>

namespace Internal {

static const char SelectCharsetDialog_lastCodecNameIniKey[] =
        "SelectCharsetDialog_lastCodecName";
static const int  SelectCharsetDialog_codecDataRole = Qt::UserRole + 1;

} // namespace Internal

SelectCharsetDialog::SelectCharsetDialog(QWidget *parent)
    : QDialog(parent),
      m_ui(new Ui_SelectCharsetDialog),
      m_filterCodecModel(new QSortFilterProxyModel(this))
{
    m_ui->setupUi(this);
    m_filterCodecModel->setFilterCaseSensitivity(Qt::CaseInsensitive);

    QObject::connect(
                m_ui->findLineEdit, &QLineEdit::textChanged,
                this, &SelectCharsetDialog::onFilterChanged);
    QObject::connect(
                m_ui->charsetListView, &QListView::doubleClicked,
                this, &SelectCharsetDialog::accept);

    QSettings appSettings;
    const QString iniLastCodecName =
            appSettings.value(
                Internal::SelectCharsetDialog_lastCodecNameIniKey,
                QLatin1String("UTF-8"))
            .toString();

    auto codecModel = new QStandardItemModel(m_filterCodecModel);
    QStandardItem* codecItemToSelect = nullptr;

    // Build sorted map of charset names
    foreach (const QByteArray& codecName, QTextCodec::availableCodecs()) {
        const QTextCodec* codec = QTextCodec::codecForName(codecName);

        // Build list of codec names
        const QString codecNameStr(codecName);
        QStringList codecAliasList(codecNameStr);
        foreach (const QByteArray& alias, codec->aliases()) {
            const QString aliasStr(alias);
            if (!codecAliasList.contains(aliasStr, Qt::CaseInsensitive))
                codecAliasList.append(aliasStr);
        }

        // Create list item for codec
        auto codecItem =
                new QStandardItem(codecAliasList.join(QLatin1String(" / ")));
        codecItem->setData(
                    codecName, Internal::SelectCharsetDialog_codecDataRole);
        codecModel->appendRow(codecItem);
        codecItem->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
        if (codecName == iniLastCodecName)
            codecItemToSelect = codecItem;
    }

    // Select the codec item lastly chosen
    m_filterCodecModel->setSourceModel(codecModel);
    m_filterCodecModel->sort(0, Qt::AscendingOrder);
    m_ui->charsetListView->setModel(m_filterCodecModel);
    if (codecItemToSelect != nullptr) {
        m_ui->charsetListView->selectionModel()->select(
                    codecItemToSelect->index(),
                    QItemSelectionModel::ClearAndSelect);
    }
}

SelectCharsetDialog::~SelectCharsetDialog()
{
    delete m_ui;
}

QByteArray SelectCharsetDialog::selectedCharset() const
{
    const QModelIndexList selectedCodecIndexes =
            m_ui->charsetListView->selectionModel()->selectedIndexes();
    const QModelIndex codecIndex =
            selectedCodecIndexes.size() == 1 ?
                selectedCodecIndexes.first() : QModelIndex();
    if (codecIndex.isValid()) {
        return codecIndex.data(Internal::SelectCharsetDialog_codecDataRole)
                .toByteArray();
    }
    return QByteArray();
}

void SelectCharsetDialog::accept()
{
    const QByteArray currentCharset = this->selectedCharset();
    if (!currentCharset.isEmpty()) {
        QSettings appSettings;
        appSettings.setValue(
                    Internal::SelectCharsetDialog_lastCodecNameIniKey,
                    currentCharset);
        QDialog::accept();
    }
    else {
        QMessageBox::information(
                    this, tr("Error"), tr("No character set selected"));
    }
}

void SelectCharsetDialog::onFilterChanged(const QString &filter)
{
    m_filterCodecModel->setFilterFixedString(filter);
}
