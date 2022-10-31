#include "createeditinputdialog.h"
#include "ui_createeditinputdialog.h"

#include <QPushButton>

CreateEditInputDialog::CreateEditInputDialog(QWidget *parent, const QStringList &outputfmts, bool isEdit) :
    QDialog(parent),
    ui(new Ui::CreateEditInputDialog)
{
    ui->setupUi(this);

    ui->comboBoxformat->insertItems(0,outputfmts);
    ui->lineEditfeederport->setValidator(new QIntValidator(1, 65535, this));
    ui->groupBox->setTitle(isEdit?"Edit Feeder Entry":"Add Feeder Entry");

    checkHostAndPort();
}

QString CreateEditInputDialog::getFormat() const
{
    return ui->comboBoxformat->currentText();
}

QString CreateEditInputDialog::getHost() const
{
    return ui->lineEditfeederhost->text();
}

QString CreateEditInputDialog::getPort() const
{
    return ui->lineEditfeederport->text();
}

void CreateEditInputDialog::setFormat(const QString &fmt)
{
    int idx=ui->comboBoxformat->findText(fmt);
    if (idx==-1) idx=ui->comboBoxformat->count()-1;
    ui->comboBoxformat->setCurrentIndex(idx);
}

void CreateEditInputDialog::setHost(const QString &host)
{
    ui->lineEditfeederhost->setText(host);
}

void CreateEditInputDialog::setPort(const QString &port)
{
    ui->lineEditfeederport->setText(port);
}

CreateEditInputDialog::~CreateEditInputDialog()
{
    delete ui;
}

void CreateEditInputDialog::checkHostAndPort()
{
    if (ui->lineEditfeederhost->text().size()>0&&ui->lineEditfeederport->text().size()>0)
        ui->buttonBox->button(QDialogButtonBox::Save)->setDisabled(false);
    else
        ui->buttonBox->button(QDialogButtonBox::Save)->setDisabled(true);
}

void CreateEditInputDialog::on_lineEditfeederhost_cursorPositionChanged(int, int)
{
    checkHostAndPort();
}


void CreateEditInputDialog::on_lineEditfeederport_cursorPositionChanged(int, int)
{
    checkHostAndPort();
}

