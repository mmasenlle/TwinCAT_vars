
#include "adscomm.h"

#include <array>
#include <vector>
#include <iostream>
#include <windows.h>

//// ADS headers for TwinCAT2
#include "C:\\TwinCAT\\AdsApi\\TcAdsDll\\Include\\TcAdsDef.h"
#include "C:\\TwinCAT\\AdsApi\\TcAdsDll\\Include\\TcAdsApi.h"

struct symbol_data
{
    QString name;
    unsigned long dataType;
    unsigned long size;
    symbol_data(QString n, unsigned long t, unsigned long s) : name(n),dataType(t),size(s) {}
};

static std::vector<struct symbol_data> symbols;

static AmsAddr Addr;
static QLabel *_label = nullptr;

void AdsComm::setAddr(const QString &s)
{
    long nErr, nPort;
    if (!_label) _label = label;
    nPort = AdsPortOpen();
    nErr = AdsGetLocalAddress(&Addr);
    if (s.size() > 10) {
        QStringList sl = s.split(".");
        for (int i = 0; i < 6 && i < sl.length(); i++) {
            Addr.netId.b[i] = sl.value(i).toInt();
        }
    }
    if (nErr) {
        QString s = "Error: AdsGetLocalAddress: " + QString::number(nErr);
        std::cerr << s.toStdString() << std::endl;
        label->setText(s);
    } else {
        label->setText("AdsGetLocalAddress(): OK");
    }
    Addr.port = 851;
}
QString AdsComm::getAddr()
{
    QString s;
    for (int i = 0; i < 6; i++) {
        s += QString::number((int)Addr.netId.b[i]);
        if (i != 5) s += ".";
    }
    return s;
}


class AdsPortOpened
{
    long nErr, nPort;
    //AmsAddr Addr;
public:
    PAmsAddr pAddr;
    AdsPortOpened(){
        pAddr = &Addr;
        // Open communication port on the ADS router
        nPort = AdsPortOpen();
    }
    ~AdsPortOpened(){
        // Close communication port
        nErr = AdsPortClose();
        if (nErr) {
            QString s = "Error: AdsPortClose: " + QString::number(nErr);
            std::cerr << s.toStdString() << std::endl;
            if (_label) _label->setText(s);
        }
    }
};

static void ads_symbols_info(QTableWidget *tw)
{
  long                  nErr;
  char                  *pchSymbols = NULL;
  UINT                  uiIndex;
  AdsSymbolUploadInfo   tAdsSymbolUploadInfo;
  PAdsSymbolEntry       pAdsSymbolEntry;

  AdsPortOpened apo;

  // Read the length of the variable declaration
  nErr = AdsSyncReadReq(apo.pAddr, ADSIGRP_SYM_UPLOADINFO, 0x0, sizeof(tAdsSymbolUploadInfo), &tAdsSymbolUploadInfo);
  if (nErr) {
      QString s = "Error: AdsSyncReadReq(ADSIGRP_SYM_UPLOADINFO): " + QString::number(nErr);
      std::cerr << s.toStdString() << std::endl;
      if (_label) _label->setText(s);
      return;
  }

  std::vector<char> symbbuffer(tAdsSymbolUploadInfo.nSymSize);
  //pchSymbols = new char[tAdsSymbolUploadInfo.nSymSize];
  pchSymbols = symbbuffer.data();

  // Read information about the PLC variables
  nErr = AdsSyncReadReq(apo.pAddr, ADSIGRP_SYM_UPLOAD, 0, tAdsSymbolUploadInfo.nSymSize, pchSymbols);
  if (nErr) {
      QString s = "Error: AdsSyncReadReq(ADSIGRP_SYM_UPLOAD): " + QString::number(nErr);
      std::cerr << s.toStdString() << std::endl;
      if (_label) _label->setText(s);
      return;
  }

  // Output information about the PLC variables
  pAdsSymbolEntry = (PAdsSymbolEntry)pchSymbols;

  std::vector<std::vector<QString>> vvs;
  for (uiIndex = 0; uiIndex < tAdsSymbolUploadInfo.nSymbols; uiIndex++)
  {
    std::cout << PADSSYMBOLNAME(pAdsSymbolEntry) << "\t\t"
         << pAdsSymbolEntry->iGroup << '\t'
         << pAdsSymbolEntry->iOffs << '\t'
         << pAdsSymbolEntry->size << '\t'
         << PADSSYMBOLTYPE(pAdsSymbolEntry) << '\t'
         << PADSSYMBOLCOMMENT(pAdsSymbolEntry) << '\n';
    std::cout.flush();

    QString vname = PADSSYMBOLNAME(pAdsSymbolEntry);

    if (!vname.startsWith("Constants") && !vname.startsWith("Global_Variables") && !vname.startsWith("TwinCAT_SystemInfoVarList")) {
        std::vector<QString> vs;
        vs.push_back(vname);
        vs.push_back(QString::number(pAdsSymbolEntry->iGroup));
        vs.push_back(QString::number(pAdsSymbolEntry->iOffs));
        vs.push_back(QString::number(pAdsSymbolEntry->dataType));
        vs.push_back(QString::number(pAdsSymbolEntry->size));
        vs.push_back(PADSSYMBOLTYPE(pAdsSymbolEntry));
        vs.push_back(PADSSYMBOLCOMMENT(pAdsSymbolEntry));
        vvs.push_back(vs);

        symbols.push_back(symbol_data(vname, pAdsSymbolEntry->dataType, pAdsSymbolEntry->size));
    }
    pAdsSymbolEntry = PADSNEXTSYMBOLENTRY(pAdsSymbolEntry);
  }

  tw->setRowCount(vvs.size());
  //getch();

  int i = 0, j = 0;
  for (auto vs : vvs) {
      j = 0;
      for (auto s : vs) {
          tw->setItem(i, j++, new QTableWidgetItem(s));
      }
      if (vs[4] == "4" || vs[4] == "2") {
          unsigned long lHdlVar;
          char data[4];
          // Fetch handle for the PLC variable
          nErr = AdsSyncReadWriteReq(apo.pAddr, ADSIGRP_SYM_HNDBYNAME, 0x0, sizeof(lHdlVar), &lHdlVar, vs[0].toStdString().size(), (void*)vs[0].toStdString().c_str());
          if (nErr) {
              QString s = "Error init 1: AdsSyncReadWriteReq: " + QString::number(nErr);
              std::cerr << s.toStdString() << std::endl;
              if (_label) _label->setText(s);
          }
          else {
              // Read values of the PLC variables (by handle)
              nErr = AdsSyncReadReq(apo.pAddr, ADSIGRP_SYM_VALBYHND, lHdlVar, vs[4].toInt(), data);
              if (nErr) {
                  QString s = "Error init 2: AdsSyncReadReq: " + QString::number(nErr);
                  std::cerr << s.toStdString() << std::endl;
                  if (_label) _label->setText(s);
              }
              else
              {
                  if (vs[5] == "REAL")
                    tw->setItem(i, j++, new QTableWidgetItem(QString::number(*reinterpret_cast<float*>(data))));
                  else if (vs[4] == "4")
                    tw->setItem(i, j++, new QTableWidgetItem(QString::number(*reinterpret_cast<int*>(data))));
                  else
                    tw->setItem(i, j++, new QTableWidgetItem(QString::number(*reinterpret_cast<short*>(data))));
              }
          }
      }
      i++;
  }
}

int type2size(unsigned long t)
{
    switch (t) {
    case 17: // byte
    case 30: // string
    case 33: // bool
        return 1;
    case 2:
    case 18:
        return 2;
    case 4:  // float
    case 19: // udint
        return 4;
    case 5:  // lreal
        return 8;
    }
    return -1;
}

static void ads_update_vars(QTableWidget *tw)
{
  long                  nErr;

  AdsPortOpened apo;

  for (int i = 0; i < symbols.size(); i++)
  {
      int ts = type2size(symbols[i].dataType);
      //std::cout << symbols[i].name.toStdString() << " " << symbols[i].dataType << " " << ts << std::endl;
      if (ts > 0) {
          unsigned long lHdlVar;
          std::array<char,256> data;
          // Fetch handle for the PLC variable
          nErr = AdsSyncReadWriteReq(apo.pAddr, ADSIGRP_SYM_HNDBYNAME, 0x0, sizeof(lHdlVar), &lHdlVar, symbols[i].name.toStdString().size(), (void*)symbols[i].name.toStdString().c_str());
          if (nErr) {
              QString s = "Error update 1: AdsSyncReadWriteReq: " + QString::number(nErr);
              std::cerr << s.toStdString() << std::endl;
              if (_label) _label->setText(s);
          }
          else {
              // Read values of the PLC variables (by handle)
              nErr = AdsSyncReadReq(apo.pAddr, ADSIGRP_SYM_VALBYHND, lHdlVar, min(symbols[i].size,data.size()), data.data());
              if (nErr) {
                  QString s = "Error update 2: AdsSyncReadReq: " + QString::number(nErr);
                  std::cerr << s.toStdString() << std::endl;
                  if (_label) _label->setText(s);
              }
              else
              {
                  //std::cout << symbols[i].name.toStdString() << " " << symbols[i].dataType << " " << ts << std::endl;
                  if (symbols[i].dataType == 30) {
//                      std::cout << "data:";
//                      for (int c : data) std::cout << " " << c;
//                      std::cout << std::endl;
//                      QString s = data.data();
//                      std::cout << "data QString("<<s.size()<<"): " << s.toStdString() << std::endl;
                      tw->setItem(i, 7, new QTableWidgetItem(QString(data.data())));
                  }
                  else if (symbols[i].dataType == 4)
                    tw->setItem(i, 7, new QTableWidgetItem(QString::number(*reinterpret_cast<float*>(data.data()))));
                  else if (symbols[i].dataType == 5)
                    tw->setItem(i, 7, new QTableWidgetItem(QString::number(*reinterpret_cast<double*>(data.data()))));
                  else if (ts > 2)
                    tw->setItem(i, 7, new QTableWidgetItem(QString::number(*reinterpret_cast<int*>(data.data()))));
                  else if (ts > 1)
                    tw->setItem(i, 7, new QTableWidgetItem(QString::number(*reinterpret_cast<short*>(data.data()))));
                  else
                    tw->setItem(i, 7, new QTableWidgetItem(QString::number(*reinterpret_cast<bool*>(data.data()))));
              }
          }
      }
  }
}

static void ads_write_val(int i, const QString &value)
{
    int ts = type2size(symbols[i].dataType);
    //std::cout << symbols[i].name.toStdString() << " " << symbols[i].dataType << " " << ts << std::endl;
    if (ts > 0) {
        AdsPortOpened apo;
        unsigned long lHdlVar;
        long nErr = AdsSyncReadWriteReq(apo.pAddr, ADSIGRP_SYM_HNDBYNAME, 0x0, sizeof(lHdlVar), &lHdlVar,
                                        symbols[i].name.toStdString().size(), (void*)symbols[i].name.toStdString().c_str());
        //if (nErr) std::cerr << "ads_write_val()->AdsSyncReadWriteReq() Error: " << nErr << '\n';
        if (nErr) {
            QString s = "ads_write_val()->AdsSyncReadWriteReq() Error: " + QString::number(nErr);
            std::cerr << s.toStdString() << std::endl;
            if (_label) _label->setText(s);
        }
        else {
            if (symbols[i].dataType == 30) {
                nErr = AdsSyncWriteReq(apo.pAddr, ADSIGRP_SYM_VALBYHND, lHdlVar, value.toStdString().size()+1, (char*)value.toStdString().c_str());
            } else if (symbols[i].dataType == 2) {
                short data = value.toShort();
                nErr = AdsSyncWriteReq(apo.pAddr, ADSIGRP_SYM_VALBYHND, lHdlVar, ts, &data);
            } else if (symbols[i].dataType == 18) {
                unsigned short data = value.toUShort();
                nErr = AdsSyncWriteReq(apo.pAddr, ADSIGRP_SYM_VALBYHND, lHdlVar, ts, &data);
            } else if (symbols[i].dataType == 4) {
                float data = value.toFloat();
                nErr = AdsSyncWriteReq(apo.pAddr, ADSIGRP_SYM_VALBYHND, lHdlVar, ts, &data);
            } else if (symbols[i].dataType == 5) {
                double data = value.toDouble();
                nErr = AdsSyncWriteReq(apo.pAddr, ADSIGRP_SYM_VALBYHND, lHdlVar, ts, &data);
            } else if (symbols[i].dataType == 19) {
                unsigned int data = value.toUInt();
                nErr = AdsSyncWriteReq(apo.pAddr, ADSIGRP_SYM_VALBYHND, lHdlVar, ts, &data);
            } else if (symbols[i].dataType == 33) {
                char data = value.toInt();
                nErr = AdsSyncWriteReq(apo.pAddr, ADSIGRP_SYM_VALBYHND, lHdlVar, ts, &data);
            }
            //if (nErr) std::cerr << "ads_write_val()->AdsSyncWriteReq() Error: " << nErr << '\n';
            if (nErr) {
                QString s = "ads_write_val()->AdsSyncWriteReq() Error: " + QString::number(nErr);
                std::cerr << s.toStdString() << std::endl;
                if (_label) _label->setText(s);
            }
        }
    }
}


AdsComm::AdsComm(QTableWidget *t) : table(t)
{

}

void AdsComm::init()
{
    symbols.clear();
    ads_symbols_info(table);
}

void AdsComm::update()
{
    ads_update_vars(table);
}

void AdsComm::change(int row, int col)
{
//    if (col != 7) {
//    std::cout << "AdsComm::change("<<row<<", "<<col<<") " << std::endl;
    if (col == 8) {
        if (table->item(row,col)) {
            std::cout << "AdsComm::change("<<row<<", "<<col<<") "<< table->item(row,col)->text().toStdString() << std::endl;
            ads_write_val(row, table->item(row,col)->text());
        }
    }
}
