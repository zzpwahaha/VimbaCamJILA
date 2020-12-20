#pragma once

#include <QDialog>
#include <QtWidgets>

class Dialog : public QDialog
{
    Q_OBJECT

public:
    Dialog();
    
private:
    void createMenu();
    void createHorizontalGroupBox();
    void createGridGroupBox();
    void createFormGroupBox();

    //enum { NumGridRows = 3, NumButtons = 4 };
    /*
    The below turns out does not work  
    int NumGridRows = 3;
    int NumButtons = 4;
    if one use this,  
    const int NumGridRows = 3;
    const int NumButtons = 4;
    would get
    'Dialog::NumGridRows': is not a type name, static, or enumerator (compiling source file main.cpp)
    this means the compiler want a static or enumerator
    so a const is not enough
    should be: 
    static const int NumGridRows = 3;
    static const int NumButtons = 4;
    */

    static const int NumGridRows = 3;
    static const int NumButtons = 4;

    QMenuBar* menuBar;
    QGroupBox* horizontalGroupBox;
    QGroupBox* gridGroupBox;
    QGroupBox* formGroupBox;
    QTextEdit* smallEditor;
    QTextEdit* bigEditor;
    QLabel* labels[NumGridRows];
    QLineEdit* lineEdits[NumGridRows];
    QPushButton* buttons[NumButtons];
    QDialogButtonBox* buttonBox;

    QMenu* fileMenu;
    QAction* exitAction;
};

//enum { NumGridRows = 3, NumButtons = 4 };