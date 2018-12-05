#ifndef ADSCOMM_H
#define ADSCOMM_H

#include <QTableWidget>
#include <QLabel>

class AdsComm : public QObject
{
    Q_OBJECT

    QTableWidget *table;

public:
    AdsComm(QTableWidget *t);

    void setAddr(const QString &s);
    QString getAddr();

    QLabel *label;

    void init();
public slots:
    void update();
    void change(int row, int col);
};

#endif // ADSCOMM_H
