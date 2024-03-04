#include "timemanager.h"
#include "./ui_timemanager.h"

TimeManager::TimeManager(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::TimeManager)
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint);

    workDate = QDate::currentDate();

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("registo_ponto.db");
    db.open();

    QSqlQuery query;
    query.exec("CREATE TABLE IF NOT EXISTS pontos (data DATE PRIMARY KEY, entrada TIME, breakManhaSaida TIME, "
               "breakManhaEntrada TIME, almocoSaida TIME, almocoEntrada TIME, breakTardeSaida TIME, breakTardeEntrada TIME, saida TIME, contador INT)");

    query.exec("SELECT COUNT(*) AS count FROM pontos");
    query.next();
    int count = query.value(0).toInt();

    if(count == 0) {
        query.prepare("INSERT INTO pontos (data, contador) VALUES (:data, :contador)");
        query.bindValue(":data", workDate.toString("yyyy-MM-dd"));
        query.bindValue(":contador", 0);
        query.exec();

        dailyCounter = 0;
    } else {
        query.exec("SELECT * FROM pontos WHERE data = '" + workDate.toString("yyyy-MM-dd") + "'");
        query.next();

        createAndSetTableItem(1, 0, query.value(1).toString(), true);
        createAndSetTableItem(0, 1, query.value(2).toString(), true);
        createAndSetTableItem(1, 1, query.value(3).toString(), true);
        createAndSetTableItem(0, 2, query.value(4).toString(), true);
        createAndSetTableItem(1, 2, query.value(5).toString(), true);
        createAndSetTableItem(0, 3, query.value(6).toString(), true);
        createAndSetTableItem(1, 3, query.value(7).toString(), true);
        createAndSetTableItem(0, 4, query.value(8).toString(), true);

        dailyCounter = query.value(9).toInt();
    }

    calculateHours();
}

TimeManager::~TimeManager()
{
    delete ui;
}

void TimeManager::createAndSetTableItem(int row, int column, const QString &text, bool isBold) {
    QTableWidgetItem *item = new QTableWidgetItem(text);
    QFont font;
    font.setBold(isBold);
    item->setFont(font);
    item->setTextAlignment(Qt::AlignCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->tableWidget->setItem(row, column, item);
}

void TimeManager::calculateHours() {
    QSqlQuery query;
    query.exec("SELECT data FROM pontos ORDER BY data DESC LIMIT 5");

    int row = 0;
    while (query.next()) {
        QString data = query.value(0).toString();

        QSqlQuery queryHoras;
        queryHoras.exec(QString("SELECT entrada, breakManhaSaida, breakManhaEntrada, almocoSaida, almocoEntrada, breakTardeSaida, breakTardeEntrada, "
                                "saida FROM pontos WHERE data = '%1'").arg(data));

        QTime total(0, 0);
        while (queryHoras.next()) {
            for(int col = 0; col < 8; col += 2) {
                QTime inicio = QTime::fromString(queryHoras.value(col).toString());
                QTime fim = QTime::fromString(queryHoras.value(col + 1).toString());

                if (!inicio.isValid() || !fim.isValid()) {
                    continue;
                }

                int intervalo = inicio.secsTo(fim);
                total = total.addSecs(intervalo);
            }
        }

        QTableWidgetItem *itemData = new QTableWidgetItem(data);
        QFont fontBold;
        fontBold.setBold(true);
        itemData->setFont(fontBold);
        itemData->setTextAlignment(Qt::AlignCenter);
        itemData->setFlags(itemData->flags() & ~Qt::ItemIsEditable);

        QTableWidgetItem *itemHoras = new QTableWidgetItem(total.toString("hh:mm"));
        itemHoras->setFont(fontBold);
        itemHoras->setTextAlignment(Qt::AlignCenter);
        itemHoras->setFlags(itemHoras->flags() & ~Qt::ItemIsEditable);

        ui->tableWidget_2->setItem(0, row, itemData);
        ui->tableWidget_2->setItem(1, row, itemHoras);

        ++row;
    }
}

void TimeManager::generatePdfReport(int selectedMonthIndex)
{
    QDate currentDate = QDate::currentDate();
    int currentYear = currentDate.year();

    if (selectedMonthIndex != 0) {
        QSqlQuery query;
        query.prepare("SELECT data, entrada, breakManhaSaida, breakManhaEntrada, almocoSaida, almocoEntrada, breakTardeSaida, breakTardeEntrada, saida FROM pontos "
                      "WHERE strftime('%m', data) = :month AND strftime('%Y', data) = :year ORDER BY data");
        query.bindValue(":month", QString("%1").arg(selectedMonthIndex, 2, 10, QChar('0')));
        query.bindValue(":year", QString::number(currentYear));
        query.exec();

        QString monthName;
        QStringList monthList = {"", "Janeiro", "Fevereiro", "Março", "Abril", "Maio", "Junho", "Julho", "Agosto", "Setembro", "Outubro", "Novembro", "Dezembro"};
        monthName = monthList[selectedMonthIndex];

        QFile file("Relatorio" + monthName + QString::number(currentYear) + ".txt");
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);

            out << "\t\t\t\t\t RELATÓRIO DO MÊS DE " + monthName.toUpper() + " DE " + QString::number(currentYear) + " \n\n";
            out << "    Data    |  Entrada  | Break Manhã S | Break Manhã E | Almoço S | Almoço E | Break Tarde S "
                   "| Break Tarde E |  Saída  | Horas Trabalhadas\n";
            while (query.next()) {
                QTime entrada = query.value(1).toTime();
                QTime saida = query.value(8).toTime();
                int segundosTrabalhados = entrada.secsTo(saida);

                segundosTrabalhados -= query.value(2).toTime().secsTo(query.value(3).toTime());
                segundosTrabalhados -= query.value(4).toTime().secsTo(query.value(5).toTime());
                segundosTrabalhados -= query.value(6).toTime().secsTo(query.value(7).toTime());

                QTime horasTrabalhadas = QTime(0, 0).addSecs(segundosTrabalhados);

                out << " " << query.value(0).toDate().toString("yyyy-MM-dd") << " |   "
                    << entrada.toString("hh:mm") << "   |    "
                    << query.value(2).toTime().toString("hh:mm") << "      |     "
                    << query.value(3).toTime().toString("hh:mm") << "     |   "
                    << query.value(4).toTime().toString("hh:mm") << "  |   "
                    << query.value(5).toTime().toString("hh:mm") << "  |      "
                    << query.value(6).toTime().toString("hh:mm") << "    |      "
                    << query.value(7).toTime().toString("hh:mm") << "    |  "
                    << saida.toString("hh:mm") << "  |      "
                    << horasTrabalhadas.toString("hh:mm") << "\n";
            }
            file.close();
            QMessageBox::information(nullptr, "Sucesso", "Relatório salvo em Relatorio" + monthName + QString::number(currentYear) + ".txt");
        } else {
            QMessageBox::critical(nullptr, "Erro", "Não foi possível criar o arquivo.");
        }
    } else {
        QMessageBox::information(nullptr, "Aviso", "Por favor, selecione um mês válido!");
    }
}

void TimeManager::updateTableItem(int row, int column, const QString &text, bool isBold) {
    QTableWidgetItem *item = new QTableWidgetItem(text);
    QFont font;
    font.setBold(isBold);
    item->setFont(font);
    item->setTextAlignment(Qt::AlignCenter);
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    ui->tableWidget->setItem(row, column, item);
    dailyCounter +=1;
}

void TimeManager::executeUpdateQuery(const QString &columnName, const QString &value) {
    QSqlQuery query;
    query.prepare(QString("UPDATE pontos SET %1 = :value, contador = :contador WHERE data = :data").arg(columnName));
    query.bindValue(":value", value);
    query.bindValue(":contador", dailyCounter);
    query.bindValue(":data", workDate);
    query.exec();
}

void TimeManager::on_pushButton_released()
{
    QSqlQuery query;
    workTime = QTime::currentTime();
    QString workTimeStr = workTime.toString("hh:mm");

    QTableWidgetItem *itemX = new QTableWidgetItem("XXXXXX");
    QTableWidgetItem *itemY = new QTableWidgetItem("XXXXXX");

    QFont fontBold;
    fontBold.setBold(true);
    itemX->setFont(fontBold);
    itemY->setFont(fontBold);

    itemX->setTextAlignment(Qt::AlignCenter);
    itemY->setTextAlignment(Qt::AlignCenter);

    itemX->setFlags(Qt::NoItemFlags);
    itemY->setFlags(Qt::NoItemFlags);

    switch (dailyCounter) {
        case 0:
            updateTableItem(1, 0, workTimeStr, true);
            executeUpdateQuery("entrada", workTimeStr);
            break;
        case 1:
            updateTableItem(0, 1, workTimeStr, true);
            executeUpdateQuery("breakManhaSaida", workTimeStr);
            break;
        case 2:
            updateTableItem(1, 1, workTimeStr, true);
            executeUpdateQuery("breakManhaEntrada", workTimeStr);
            break;
        case 3:
            updateTableItem(0, 2, workTimeStr, true);
            executeUpdateQuery("almocoSaida", workTimeStr);
            break;
        case 4:
            updateTableItem(1, 2, workTimeStr, true);
            executeUpdateQuery("almocoEntrada", workTimeStr);
            break;
        case 5:
            updateTableItem(0, 3, workTimeStr, true);
            executeUpdateQuery("breakTardeSaida", workTimeStr);
            break;
        case 6:
            updateTableItem(1, 3, workTimeStr, true);
            executeUpdateQuery("breakTardeEntrada", workTimeStr);
            break;
        case 7:
            updateTableItem(0, 4, workTimeStr, true);
            executeUpdateQuery("saida", workTimeStr);
            break;
        case 8:
            ui->tableWidget->clearContents();
            ui->tableWidget->setItem(0, 0, itemX);
            ui->tableWidget->setItem(1, 4, itemY);

            QDate tomorrowWork = workDate.addDays(1);
            query.prepare("INSERT INTO pontos (data, contador) VALUES (:data, :contador)");
            query.bindValue(":data", tomorrowWork.toString("yyyy-MM-dd"));
            query.bindValue(":contador", 0);
            query.exec();

            QMessageBox::information(nullptr, "Fim do Dia de Trabalho", "Parabéns por ter sobrevivido mais um dia!");
            calculateHours();
            break;
    }
}

void TimeManager::on_pushButton_2_released()
{
    reject();
}

void TimeManager::on_pushButton_3_released()
{
    generatePdfReport(ui->comboBox->currentIndex());
}
