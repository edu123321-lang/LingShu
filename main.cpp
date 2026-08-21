#include "widget.h"
#include "mycamera.h"
#include <QApplication>
#include <QDir>
#include <QDebug>
#include <QStandardPaths>
#include <QFont>
#include <QFontDatabase>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QFont defaultFont("Microsoft YaHei UI", 10);
    defaultFont.setHintingPreference(QFont::PreferFullHinting);
    a.setFont(defaultFont);

    MyCamera::setPreferredCameraByName("Web Camera");
    // MyCamera::setPreferredCameraByIndex(0);


    QString qss = R"(

* {
    font-family: "Microsoft YaHei UI", "PingFang SC", "Segoe UI", sans-serif;
}

QWidget {
    color: #2d3748;
}

QWidget#Widget {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
        stop:0 #0f2027,
        stop:0.5 #203a43,
        stop:1 #2c5364);
}

QWidget#Gmwidget, QWidget#UserWidget {
    background-color: #f0f4f8;
}

QWidget#leftWidget {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
        stop:0 #1e3a5f,
        stop:1 #0f2744);
    border-right: 1px solid rgba(255,255,255,0.08);
}

QWidget#leftWidget QLabel {
    color: #e2e8f0;
}

QLabel#titleLabel {
    font-size: 42px;
    font-weight: 700;
    color: #ffffff;
    letter-spacing: 4px;
    padding-top: 10px;
}

QLabel#subtitleLabel {
    font-size: 15px;
    color: rgba(255,255,255,0.65);
    letter-spacing: 3px;
    padding-top: 8px;
}

QLabel#gmFormTitleLabel, QLabel#userFormTitleLabel {
    font-size: 22px;
    font-weight: 600;
    color: #1a365d;
    margin-bottom: 10px;
}

QPushButton#GmBtn, QPushButton#UserBtn {
    background-color: rgba(255,255,255,0.08);
    color: #ffffff;
    border: 1.5px solid rgba(255,255,255,0.25);
    border-radius: 14px;
    font-size: 16px;
    font-weight: 500;
    padding: 10px 24px;
}
QPushButton#GmBtn:hover, QPushButton#UserBtn:hover {
    background-color: rgba(255,255,255,0.18);
    border-color: rgba(255,255,255,0.5);
}
QPushButton#GmBtn:checked, QPushButton#UserBtn:checked {
    background-color: #38b2ac;
    border-color: #38b2ac;
    color: #ffffff;
}

QStackedWidget#stackedWidget {
    background-color: #ffffff;
    border-radius: 20px;
}

QStackedWidget#stackedWidget > QWidget {
    background-color: #ffffff;
    border-radius: 20px;
}

QLineEdit {
    background-color: #f7fafc;
    border: 1.5px solid #e2e8f0;
    border-radius: 10px;
    padding: 10px 14px;
    font-size: 14px;
    color: #2d3748;
    selection-background-color: #38b2ac;
}
QLineEdit:hover {
    border-color: #cbd5e0;
    background-color: #ffffff;
}
QLineEdit:focus {
    border-color: #38b2ac;
    background-color: #ffffff;
}
QLineEdit::placeholder {
    color: #a0aec0;
}

QPushButton#GmLoginBtn, QPushButton#userLoginBtn {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #319795,
        stop:1 #38b2ac);
    color: #ffffff;
    border: none;
    border-radius: 12px;
    font-size: 15px;
    font-weight: 600;
    padding: 10px 24px;
}
QPushButton#GmLoginBtn:hover, QPushButton#userLoginBtn:hover {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #2c7a7b,
        stop:1 #319795);
}
QPushButton#GmLoginBtn:pressed, QPushButton#userLoginBtn:pressed {
    padding-top: 11px;
    padding-bottom: 9px;
}

QWidget#leftWidget QPushButton {
    background-color: transparent;
    color: rgba(226, 232, 240, 0.85);
    border: none;
    border-radius: 10px;
    font-size: 14px;
    text-align: left;
    padding-left: 20px;
    margin: 2px 0px;
}
QWidget#leftWidget QPushButton:hover {
    background-color: rgba(255,255,255,0.08);
    color: #ffffff;
}
QWidget#leftWidget QPushButton:checked {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #319795,
        stop:1 #38b2ac);
    color: #ffffff;
    font-weight: 600;
}

QPushButton#logoutBtn {
    background-color: rgba(254, 178, 178, 0.15) !important;
    color: #feb2b2 !important;
    border: 1px solid rgba(254, 178, 178, 0.3) !important;
    margin-top: 20px !important;
}
QPushButton#logoutBtn:hover {
    background-color: rgba(245, 101, 101, 0.2) !important;
    color: #fff5f5 !important;
}

QPushButton {
    background-color: #edf2f7;
    border: 1.5px solid #e2e8f0;
    border-radius: 10px;
    padding: 8px 18px;
    font-size: 14px;
    color: #2d3748;
    font-weight: 500;
}
QPushButton:hover {
    background-color: #e2e8f0;
    border-color: #cbd5e0;
}
QPushButton:pressed {
    background-color: #cbd5e0;
}

QPushButton[flat="true"] {
    background-color: transparent;
    border: none;
    padding: 6px 12px;
    color: #4a5568;
}
QPushButton[flat="true"]:hover {
    background-color: #edf2f7;
    border-radius: 8px;
    color: #2d3748;
}

QTableWidget, QTableView {
    background-color: #ffffff;
    border: 1px solid #e2e8f0;
    border-radius: 12px;
    gridline-color: #edf2f7;
    selection-background-color: rgba(56, 178, 172, 0.15);
    selection-color: #234e52;
    font-size: 13px;
}
QHeaderView::section {
    background-color: #f7fafc;
    color: #4a5568;
    padding: 10px 12px;
    border: none;
    border-bottom: 1.5px solid #e2e8f0;
    font-weight: 600;
    font-size: 13px;
}
QTableWidget::item, QTableView::item {
    padding: 8px 12px;
    border-bottom: 1px solid #f0f4f8;
}
QTableWidget::item:selected, QTableView::item:selected {
    background-color: rgba(56, 178, 172, 0.1);
    color: #234e52;
}
QScrollBar:vertical {
    background: #f7fafc;
    width: 10px;
    margin: 4px;
    border-radius: 5px;
}
QScrollBar::handle:vertical {
    background: #cbd5e0;
    min-height: 30px;
    border-radius: 5px;
}
QScrollBar::handle:vertical:hover {
    background: #a0aec0;
}
QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
    height: 0;
}
QScrollBar:horizontal {
    background: #f7fafc;
    height: 10px;
    margin: 4px;
    border-radius: 5px;
}
QScrollBar::handle:horizontal {
    background: #cbd5e0;
    min-width: 30px;
    border-radius: 5px;
}
QScrollBar::handle:horizontal:hover {
    background: #a0aec0;
}
QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
    width: 0;
}

QComboBox {
    background-color: #f7fafc;
    border: 1.5px solid #e2e8f0;
    border-radius: 10px;
    padding: 8px 14px;
    font-size: 14px;
    min-height: 20px;
}
QComboBox:hover {
    border-color: #cbd5e0;
    background-color: #ffffff;
}
QComboBox:focus {
    border-color: #38b2ac;
    background-color: #ffffff;
}
QComboBox::drop-down {
    subcontrol-origin: padding;
    subcontrol-position: top right;
    width: 30px;
    border: none;
    border-top-right-radius: 10px;
    border-bottom-right-radius: 10px;
}
QComboBox::down-arrow {
    image: none;
    border-left: 5px solid transparent;
    border-right: 5px solid transparent;
    border-top: 5px solid #718096;
    margin-right: 10px;
}
QComboBox QAbstractItemView {
    border: 1px solid #e2e8f0;
    border-radius: 10px;
    background-color: #ffffff;
    selection-background-color: rgba(56, 178, 172, 0.15);
    selection-color: #234e52;
    outline: 0;
    padding: 6px;
}

QRadioButton {
    spacing: 8px;
    color: #4a5568;
    font-size: 14px;
}
QRadioButton::indicator {
    width: 18px;
    height: 18px;
    border-radius: 9px;
    border: 2px solid #cbd5e0;
    background-color: #ffffff;
}
QRadioButton::indicator:hover {
    border-color: #a0aec0;
}
QRadioButton::indicator:checked {
    border-color: #38b2ac;
    background-color: #38b2ac;
}
QRadioButton::indicator:checked::indicator {
    width: 8px;
    height: 8px;
    border-radius: 4px;
    background-color: #ffffff;
}

QCheckBox {
    spacing: 8px;
    color: #4a5568;
    font-size: 14px;
}
QCheckBox::indicator {
    width: 18px;
    height: 18px;
    border-radius: 5px;
    border: 2px solid #cbd5e0;
    background-color: #ffffff;
}
QCheckBox::indicator:hover {
    border-color: #a0aec0;
}
QCheckBox::indicator:checked {
    background-color: #38b2ac;
    border-color: #38b2ac;
}

QSpinBox, QDoubleSpinBox, QDateEdit, QTimeEdit, QDateTimeEdit {
    background-color: #f7fafc;
    border: 1.5px solid #e2e8f0;
    border-radius: 10px;
    padding: 6px 10px;
    font-size: 14px;
    min-height: 20px;
}
QSpinBox:hover, QDoubleSpinBox:hover, QDateEdit:hover, QTimeEdit:hover, QDateTimeEdit:hover {
    border-color: #cbd5e0;
    background-color: #ffffff;
}
QSpinBox:focus, QDoubleSpinBox:focus, QDateEdit:focus, QTimeEdit:focus, QDateTimeEdit:focus {
    border-color: #38b2ac;
    background-color: #ffffff;
}

QTabWidget::pane {
    border: 1px solid #e2e8f0;
    border-radius: 12px;
    background-color: #ffffff;
    top: -1px;
}
QTabBar::tab {
    background-color: #f7fafc;
    border: 1px solid #e2e8f0;
    border-bottom: none;
    border-top-left-radius: 10px;
    border-top-right-radius: 10px;
    padding: 10px 22px;
    margin-right: 4px;
    color: #4a5568;
    font-size: 14px;
    font-weight: 500;
}
QTabBar::tab:hover {
    background-color: #edf2f7;
    color: #2d3748;
}
QTabBar::tab:selected {
    background-color: #ffffff;
    color: #319795;
    border-color: #38b2ac;
    border-bottom: 2px solid #38b2ac;
    font-weight: 600;
}

QMenu {
    background-color: #ffffff;
    border: 1px solid #e2e8f0;
    border-radius: 10px;
    padding: 6px;
}
QMenu::item {
    padding: 8px 20px;
    border-radius: 6px;
    color: #2d3748;
    font-size: 14px;
}
QMenu::item:selected {
    background-color: rgba(56, 178, 172, 0.12);
    color: #234e52;
}
QMenu::separator {
    height: 1px;
    background: #edf2f7;
    margin: 4px 10px;
}

QToolTip {
    background-color: #2d3748;
    color: #ffffff;
    border: none;
    border-radius: 6px;
    padding: 6px 10px;
    font-size: 13px;
}

QStatusBar {
    background-color: #ffffff;
    border-top: 1px solid #e2e8f0;
    color: #718096;
    font-size: 13px;
}

QProgressBar {
    background-color: #edf2f7;
    border: none;
    border-radius: 6px;
    text-align: center;
    height: 12px;
}
QProgressBar::chunk {
    background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
        stop:0 #319795,
        stop:1 #38b2ac);
    border-radius: 6px;
}

QTextEdit, QPlainTextEdit {
    background-color: #ffffff;
    border: 1.5px solid #e2e8f0;
    border-radius: 10px;
    padding: 10px 14px;
    font-size: 14px;
    color: #2d3748;
}
QTextEdit:hover, QPlainTextEdit:hover {
    border-color: #cbd5e0;
}
QTextEdit:focus, QPlainTextEdit:focus {
    border-color: #38b2ac;
}

QLabel[hyperlink="true"] {
    color: #319795;
    text-decoration: underline;
}
QLabel[hyperlink="true"]:hover {
    color: #285e61;
}

QGroupBox {
    border: 1px solid #e2e8f0;
    border-radius: 12px;
    margin-top: 20px;
    padding-top: 20px;
    background-color: #ffffff;
    font-size: 14px;
    font-weight: 600;
    color: #1a365d;
}
QGroupBox::title {
    subcontrol-origin: margin;
    left: 18px;
    padding: 0px 8px;
    color: #1a365d;
}

QScrollArea {
    background-color: transparent;
    border: none;
}
QScrollArea > QWidget > QWidget {
    background-color: transparent;
}

QFrame[cardStyle="true"] {
    background-color: #ffffff;
    border: 1px solid #e2e8f0;
    border-radius: 14px;
}

    )";

    a.setStyleSheet(qss);

    QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(dataDir);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QDir attachmentsDir(dataDir + "/attachments");
    if (!attachmentsDir.exists()) {
        attachmentsDir.mkpath(".");
    }

    Widget w;
    w.show();

    return a.exec();
}
