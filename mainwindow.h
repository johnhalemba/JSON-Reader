#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QResizeEvent>
#include <QJsonDocument>
#include <QFileDialog>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QJsonObject>
#include <QJsonArray>
#include <QInputDialog>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QComboBox>
#include <QLabel>
#include <QWidgetAction>
#include <QPushButton>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

protected:
    void resizeEvent(QResizeEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void openAndLoadFile();
    void addButtonTriggered();
    void deleteButtonTriggered();
    void newButtonTriggered();
    void saveToFile();
    void alternativeAddButton();

private:
    Ui::MainWindow *ui;
    QJsonDocument document;
    QJsonDocument emptyDocument;
    void addJsonDataToTreeWidget(const QVariant &data, QTreeWidgetItem *parentItem);
    void setItemType(QTreeWidgetItem *item, const QString &type);
    QString filePath;
    int fileNotSaved();
    //void setItemsEditable(QTreeWidgetItem *item);
    QJsonValue itemToJsonValue(QTreeWidgetItem *item);
    QJsonValue treeToJsonValue(QTreeWidget *treeWidget);
    void alternateAddToTreeWidget(const QJsonDocument& document, QTreeWidgetItem* rootItem);
    void addJsonToTree(const QJsonObject& jsonObj, QTreeWidgetItem* parentItem);
    int newTypePopUp();
    bool fileOpened = false;
    bool newFile = false;
    bool treeWidgetChanged = false;
    bool fileSaved = false;
    bool arrayLoaded = false;
    bool objectLoaded = false;
};
#endif // MAINWINDOW_H
