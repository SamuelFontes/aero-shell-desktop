/* SPDX-License-Identifier: MIT */
#include "taskmanager.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QHeaderView>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>
#include <QTableWidget>
#include <QTimer>
#include <csignal>
#include <cerrno>
#include <cstring>
#include <unistd.h>
TaskManager::TaskManager()
{
    setAttribute(Qt::WA_DeleteOnClose);
    setWindowTitle(tr("Windows Task Manager"));
    resize(650, 480);
    auto *table = new QTableWidget(0, 4, this);
    table->setHorizontalHeaderLabels({tr("Image Name"), tr("PID"), tr("User ID"), tr("Memory (KiB)")});
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    setCentralWidget(table);
    auto refresh = [table] {
        const QString selected = table->currentRow() >= 0 ? table->item(table->currentRow(), 1)->text() : QString{};
        table->setRowCount(0);
        for (const auto &entry : QDir(QStringLiteral("/proc")).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            bool numeric = false;
            entry.toInt(&numeric);
            if (!numeric) continue;
            QFile status(QStringLiteral("/proc/%1/status").arg(entry));
            if (!status.open(QIODevice::ReadOnly)) continue;
            QString name, uid, memory;
            for (const auto &line : status.readAll().split('\n')) {
                if (line.startsWith("Name:")) name = QString::fromUtf8(line.mid(5)).trimmed();
                if (line.startsWith("Uid:")) uid = QString::fromUtf8(line.mid(4)).simplified().section(' ', 0, 0);
                if (line.startsWith("VmRSS:")) memory = QString::fromUtf8(line.mid(6)).simplified().section(' ', 0, 0);
            }
            const int row = table->rowCount();
            table->insertRow(row);
            const QStringList fields{name, entry, uid, memory.isEmpty() ? QStringLiteral("0") : memory};
            for (int col = 0; col < fields.size(); ++col) table->setItem(row, col, new QTableWidgetItem(fields.at(col)));
            if (entry == selected) table->selectRow(row);
        }
    };
    auto *end = new QPushButton(tr("End Process…"));
    statusBar()->addPermanentWidget(end);
    connect(end, &QPushButton::clicked, this, [this, table] {
        if (table->currentRow() < 0) return;
        const int pid = table->item(table->currentRow(), 1)->text().toInt();
        if (pid <= 1 || pid == getpid()) return;
        if (QMessageBox::question(this, tr("End Process"), tr("End %1? Unsaved work may be lost.").arg(table->item(table->currentRow(), 0)->text()),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes && ::kill(pid, SIGTERM) != 0) {
            QMessageBox::warning(this, tr("Could not end process"), QString::fromLocal8Bit(std::strerror(errno)));
        }
    });
    auto *timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, refresh);
    timer->start(2000);
    refresh();
}
