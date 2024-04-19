#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->centralwidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    ui->treeWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui->treeWidget->setHeaderLabels(QStringList() << "Key" << "Value");
    ui->treeWidget->setColumnCount(2);

    QLabel* label = new QLabel("test");

    // Create a QWidgetAction and set its default widget to the label
    QWidgetAction* action = new QWidgetAction(this);
    action->setDefaultWidget(label);

    // Add the action to the menu bar
    menuBar()->addAction(action);


    connect(ui->actionOpen, &QAction::triggered, this, &MainWindow::openAndLoadFile);
    connect(ui->actionAdd, &QAction::triggered, this, &MainWindow::alternativeAddButton);
    connect(ui->actionRemove, &QAction::triggered, this, &MainWindow::deleteButtonTriggered);
    connect(ui->actionNew, &QAction::triggered, this, &MainWindow::newButtonTriggered);
    connect(ui->actionSave, &QAction::triggered, this, &MainWindow::saveToFile);
    connect(ui->actionSaveAs, &QAction::triggered, this, &MainWindow::saveToFile);

    connect(ui->treeWidget, &QTreeWidget::itemDoubleClicked, [=](QTreeWidgetItem* item, int column) {
        if (item->text(0).startsWith("[Element]") && item->childCount() > 0) {
            QMessageBox::critical(this, tr("Error!"), "This element has already some objects/arrays in it!");
            return;
        }
        QDialog editDialog(this);
        editDialog.setWindowTitle("Edit Item");

        QFormLayout* editLayout = new QFormLayout(&editDialog);
        QString tempKey = item->text(0);
        QString finalString = item->text(0);
        if (tempKey.startsWith("[Object]")) {
            finalString.remove(0, QString("[Object] ").size());
        } else if (tempKey.startsWith("[Element]")) {
            if (item->childCount() > 0) {
                QMessageBox::critical(this, tr("Error!"), "This element has already some objects/arrays in it!");
                delete editLayout;
                // delete editDialog;
                return;
            }
            finalString.remove(0, QString("[Element] ").size());
        } else if (tempKey.startsWith("[Array]")) {
            finalString.remove(0, QString("[Array] ").size());
        }

        QLineEdit* keyEdit = new QLineEdit(finalString, &editDialog);
        QLineEdit* valueEdit = new QLineEdit(item->text(1), &editDialog);

        if (tempKey.startsWith("[Element]")) {
            editLayout->addRow("Value:", valueEdit);
            delete keyEdit;
        } else if (tempKey.startsWith("[Array]")) {
            editLayout->addRow("Key:", keyEdit);
            delete valueEdit;
        } else {
            editLayout->addRow("Key:", keyEdit);
            editLayout->addRow("Value:", valueEdit);
        }

        QDialogButtonBox* buttonBox = new QDialogButtonBox(Qt::Horizontal, &editDialog);
        QPushButton* okButton = buttonBox->addButton("OK", QDialogButtonBox::AcceptRole);
        QPushButton* cancelButton = buttonBox->addButton("Cancel", QDialogButtonBox::RejectRole);
        editLayout->addRow(buttonBox);

        connect(okButton, &QPushButton::clicked, &editDialog, &QDialog::accept);
        connect(cancelButton, &QPushButton::clicked, &editDialog, &QDialog::reject);

        if (editDialog.exec() == QDialog::Accepted) {
            if (tempKey.startsWith("[Object]")) {
                item->setText(0, QString("[Object] %1").arg(keyEdit->text()));
                item->setText(1, valueEdit->text());
            } else if (tempKey.startsWith("[Element]")) {
                item->setText(1, valueEdit->text());
            } else if (tempKey.startsWith("[Array]")) {
                item->setText(0, QString("[Array] %1").arg(keyEdit->text()));
            }
        } else {
            return;
        }
    });

    ui->treeWidget->viewport()->installEventFilter(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event);
    qDebug() << "treeWidget size: " << ui->treeWidget->size();
    int margin = 20;
    QSize centralWidgetSize = ui->centralwidget->size();
    ui->treeWidget->resize(centralWidgetSize - QSize(margin, margin));
}

void MainWindow::openAndLoadFile() {
    int MessageBoxAnswer;
    if (newFile || fileOpened)
        MessageBoxAnswer = fileNotSaved();

    filePath = QFileDialog::getOpenFileName(this, tr("Open file"), QDir::homePath(), tr("JSON files (*.json);;All files (*.*)"));
    QByteArray jsonRawData;

    if (fileOpened || newFile) {
        if (MessageBoxAnswer == 0) {
            ui->treeWidget->clear();
            document = emptyDocument;
            this->setWindowTitle("JSON reader");
            newFile = false;
        } else {
            saveToFile();
            ui->treeWidget->clear();
            document = emptyDocument;
            this->setWindowTitle("JSON reader");
            newFile = false;
        }
    }

    if(filePath.isEmpty()) {
        return;
    } else {
        qDebug() << "Opening path: " << filePath;
        QFile file(filePath);
        fileOpened = file.open(QIODevice::ReadOnly | QIODevice::Text);
        if (!fileOpened) {
            QMessageBox::critical(this, tr("Error!"), "Error opening file!");
            return;
        }
        jsonRawData = file.readAll();
        file.close();
    }

    QJsonParseError error;
    document = QJsonDocument::fromJson(jsonRawData, &error);
    if (document.isNull()) {
        QMessageBox::critical(this, tr("Error!"), "Error parsing JSON: " + error.errorString());
    }

    if (document.isArray())
        arrayLoaded = true;
    else
        objectLoaded = true;

    this->setWindowTitle(filePath);

    qDebug() << &"isArray(): " [ document.isArray()];
    qDebug() << &"isObject(): " [ document.isObject()];
    QVariant jsonVariant = document.toVariant();

    addJsonDataToTreeWidget(jsonVariant, ui->treeWidget->invisibleRootItem());

    ui->actionSave->setEnabled(true);
    ui->actionSaveAs->setEnabled(true);
    ui->actionAdd->setEnabled(true);
    ui->actionRemove->setEnabled(true);
    ui->menuEdit->setEnabled(true);
}

void MainWindow::addJsonDataToTreeWidget(const QVariant &data, QTreeWidgetItem *parentItem) {
    if (data.type() == QVariant::Map) {
        // adding keys as top-level items
        QMap<QString, QVariant> map = data.toMap();
        for (const QString &key : map.keys()) {
            auto *item = new QTreeWidgetItem;
            if (map[key].type() == QVariant::List) {
                item->setText(0, QString("[Array] %1").arg(key));
            } else {
                item->setText(0, QString("[Object] %1").arg(key));
            }
            parentItem->addChild(item);
            addJsonDataToTreeWidget(map[key], item);
        }
    } else if (data.type() == QVariant::List) {
        // adding list items as child items
        QList<QVariant> list = data.toList();
        int counter = 0;
        for (const QVariant &itemData : list) {
            auto *item = new QTreeWidgetItem;
            parentItem->addChild(item);
            item->setText(0, QString("[Element] %1").arg(counter));
            addJsonDataToTreeWidget(itemData, item);
            counter++;
        }
    } else {
        parentItem->setText(1, data.toString());
    }
}

//void MainWindow::setItemsEditable(QTreeWidgetItem *item) {
//    item->setFlags(item->flags() | Qt::ItemIsEditable);
//    for (int i = 0; i < item->childCount(); ++i) {
//        QTreeWidgetItem *childItem = item->child(i);
//        setItemsEditable(childItem);
//    }
//}

void MainWindow::addButtonTriggered() {
    QTreeWidgetItem *parentItem = ui->treeWidget->currentItem();
    if (!parentItem) {
        auto *newItem = new QTreeWidgetItem(ui->treeWidget);
        newItem->setText(0, "New Key");
        newItem->setText(1, "New Value");
        ui->treeWidget->addTopLevelItem(newItem);
        return;
    } else {
        QString prefix = parentItem->text(0).left(8);
        if (prefix == "[Object]") {
            auto *newChildItem = new QTreeWidgetItem(parentItem);
            newChildItem->setText(0, "New Child Key");
            newChildItem->setText(1, "New Child Value");
            parentItem->addChild(newChildItem);
            parentItem->setExpanded(true);
        } else if (prefix == "[Array] "){
            auto *newChildItem = new QTreeWidgetItem(parentItem);
            newChildItem->setText(0, QString("%1").arg(parentItem->childCount() - 1));
            newChildItem->setText(1, "New array value");
            parentItem->addChild(newChildItem);
            parentItem->setExpanded(true);
        }
    }
}

void MainWindow::alternativeAddButton() {
    auto* parentItem = ui->treeWidget->currentItem();

    if (arrayLoaded && parentItem == nullptr) {
        auto *newItem = new QTreeWidgetItem(ui->treeWidget);
        newItem->setText(0, QString("[Element] %1").arg(ui->treeWidget->topLevelItemCount() - 1));
        ui->treeWidget->addTopLevelItem(newItem);
        return;
    }

    if (ui->treeWidget->topLevelItemCount() > 0 && parentItem != nullptr) {
        if (parentItem->text(0).left(7) == "[Array]") {
            auto *newItem = new QTreeWidgetItem(parentItem);
            newItem->setText(0, QString("[Element] %1").arg(parentItem->childCount() - 1));
            parentItem->addChild(newItem);
            return;
        }
    }

    QLineEdit *keyLineEdit = new QLineEdit;
    QLineEdit *valueLineEdit = new QLineEdit;

    QComboBox *comboBox = new QComboBox;

    comboBox->addItem("Object");
    comboBox->addItem("Array");


    QFormLayout *layout = new QFormLayout;
    layout->addRow("Key:", keyLineEdit);
    layout->addRow("Value:", valueLineEdit);
    layout->addRow("Type:", comboBox);

    connect(comboBox, &QComboBox::textActivated, [=](const QString& text) {
        QLayoutItem* item = layout->itemAt(1, QFormLayout::FieldRole);
        QWidget* widget = item->widget();
        if (text == "Array") {
            if (widget) {
                widget->setEnabled(false);
            }
        } else {
            if (widget) {
                widget->setEnabled(true);
            }
        }
    });

    QDialog *dialog = new QDialog(this);
    dialog->setLayout(layout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
    layout->addRow(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::reject);

    int dialogAnswer = dialog->exec();

    if (dialogAnswer == 1) {
        QString selectedText = comboBox->currentText();
        if (!parentItem) {
            auto *newItem = new QTreeWidgetItem(ui->treeWidget);
            if (selectedText == "Object") {
                newItem->setText(0, QString("[Object] %1").arg(keyLineEdit->text()));
                newItem->setText(1, QString("%1").arg(valueLineEdit->text()));
            } else if (selectedText == "Array") {
                newItem->setText(0, QString("[Array] %1").arg(keyLineEdit->text()));
            }
            ui->treeWidget->addTopLevelItem(newItem);
            return;
        } else {
            auto *newChildItem = new QTreeWidgetItem(parentItem);
            if (selectedText == "Object") {
                newChildItem->setText(0, QString("[Object] %1").arg(keyLineEdit->text()));
                newChildItem->setText(1, valueLineEdit->text());
            } else if (selectedText == "Array") {
                newChildItem->setText(0, QString("[Array] %1").arg(keyLineEdit->text()));
            }
            parentItem->addChild(newChildItem);
            parentItem->setText(1, "");
        }
    }
}

void MainWindow::deleteButtonTriggered() {
    auto *item = ui->treeWidget->currentItem();
    if (!item)
        return;

    if (item->childCount() > 0) {
        for (int i = 0; i<item->childCount(); i++) {
            auto *toDelete = item->child(i);
            delete toDelete;
        }
    }

    auto *parent = item->parent();

    if (parent) {
        parent->takeChild(parent->indexOfChild(item));
        delete item;
    } else {
        ui->treeWidget->takeTopLevelItem(ui->treeWidget->indexOfTopLevelItem(item));
        delete item;
    }
}

int MainWindow::newTypePopUp() {
    QMessageBox msgBox;
    msgBox.setText("What is the top-level JSON type?");
    msgBox.addButton("Array", QMessageBox::YesRole);
    msgBox.addButton("Object", QMessageBox::NoRole);
    return msgBox.exec();
}

void MainWindow::newButtonTriggered() {
    if (fileOpened || newFile) {
        int MessageBoxAnswer = fileNotSaved();
        if (MessageBoxAnswer == 0) {
            ui->treeWidget->clear();
            document = emptyDocument;
            arrayLoaded = false;
            objectLoaded = false;
            this->setWindowTitle("JSON reader");
            fileOpened = false;
        } else {
            //save file
            return;
        }
    }

    int JsonType = newTypePopUp();

    if (JsonType == 0) {
        arrayLoaded = true;
    } else {
        objectLoaded = true;
    }

    newFile = true;
    alternativeAddButton();
    ui->actionSave->setEnabled(true);
    ui->actionSaveAs->setEnabled(true);
    ui->actionAdd->setEnabled(true);
    ui->actionRemove->setEnabled(true);
    ui->menuEdit->setEnabled(true);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == ui->treeWidget->viewport() && event->type() == QEvent::MouseButtonPress && ui->treeWidget->currentItem() != nullptr) {
        ui->treeWidget->clearSelection();
        ui->treeWidget->setCurrentItem(nullptr);
    }

    return QObject::eventFilter(watched, event);
}

int MainWindow::fileNotSaved() {
    QMessageBox msgBox;
    msgBox.setWindowTitle("Confirmation");
    msgBox.setText("Do you want to close file without saving?");
    msgBox.setIcon(QMessageBox::Warning);

    auto *yesButton = msgBox.addButton(tr("Yes"), QMessageBox::YesRole);
    auto *noButton = msgBox.addButton(tr("No"), QMessageBox::NoRole);

    msgBox.setDefaultButton(noButton);

    return msgBox.exec();
}

QJsonValue MainWindow::itemToJsonValue(QTreeWidgetItem *item) {
    if (item->childCount() > 0 && item->text(0).startsWith("[Object]")) {
        QJsonObject obj;
        for (int j = 0; j < item->childCount(); j++) {
            QTreeWidgetItem *childItem = item->child(j);
            obj[childItem->text(0).remove(0, QString("[Object] ").size())] = itemToJsonValue(childItem);
        }
        return QJsonValue(obj);
    } else if (item->childCount() > 0 && item->text(0).startsWith("[Array]")) {
        QJsonArray arr;
        for (int j = 0; j < item->childCount(); j++) {
            QTreeWidgetItem *childItem = item->child(j);
            arr.append(itemToJsonValue(childItem));
        }
        return QJsonValue(arr);
    } else if (item->childCount() == 0 && item->text(0).startsWith("[Element]")) {
        return QJsonValue(item->text(1));
    } else if (item->childCount() == 0 && item->text(0).startsWith("[Object]")) {
//        QJsonObject obj;
//        //QTreeWidgetItem *childItem = item->child(0);
//        obj[item->text(0)] = item->text(1);
        return QJsonValue(item->text(1));
    } else {
        QJsonObject obj;
        for (int j = 0; j < item->childCount(); j++) {
            QTreeWidgetItem *childItem = item->child(j);
            obj[childItem->text(0).remove(0, QString("[Object ]").size())] = itemToJsonValue(childItem);
        }
        return QJsonValue(obj);
    }
}

QJsonValue MainWindow::treeToJsonValue(QTreeWidget *treeWidget) {
    if (arrayLoaded) {
        QJsonArray arr;
        for (int i = 0; i < treeWidget->topLevelItemCount(); i++) {
            QTreeWidgetItem *item = treeWidget->topLevelItem(i);
            arr.append(itemToJsonValue(item));
        }
        return QJsonValue(arr);
    } else if (objectLoaded){
        QJsonObject obj;
        for (int i = 0; i < treeWidget->topLevelItemCount(); i++) {
            QTreeWidgetItem *item = treeWidget->topLevelItem(i);
            obj[item->text(0)] = itemToJsonValue(item);
        }
        return QJsonValue(obj);
    }
}

void MainWindow::saveToFile() {
    QJsonValue topLevelValue = treeToJsonValue(ui->treeWidget);
    QJsonDocument jsonDoc;
    // Create a JSON document from the JSON object or array
    if (arrayLoaded) {
        jsonDoc.setArray(topLevelValue.toArray());
    } else if (objectLoaded) {
        jsonDoc.setObject(topLevelValue.toObject());
    }

    QObject* sender = QObject::sender();

    QString saveFileName;
    if (sender->objectName() == QString("actionSave")) {
        saveFileName = filePath;
    } else if (sender->objectName() == QString("actionSaveAs")) {
        saveFileName = QFileDialog::getSaveFileName(this, tr("Save JSON file"), QDir::homePath(), tr("JSON files (*.json)"));
    }

    QFile file(saveFileName);
    fileSaved = file.open(QIODevice::WriteOnly | QIODevice::Text);
    if (!fileSaved) {
        QMessageBox::critical(this, tr("Error!"), "Error saving file!");
        return;
    }
    file.write(jsonDoc.toJson(QJsonDocument::Indented));
    file.close();

    ui->actionSave->setEnabled(false);
    ui->actionSaveAs->setEnabled(false);
}
