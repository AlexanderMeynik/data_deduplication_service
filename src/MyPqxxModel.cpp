#include "MyPqxxModel.h"


MyPqxxModel::MyPqxxModel(myConnString &cstring, QObject *parent) : QAbstractTableModel(parent) {
    this->connection_ = connectIfPossible(cstring.c_str()).value_or(nullptr);
    good = db_services::checkConnection(this->connection_);
    res = pqxx::result();
}

QVariant MyPqxxModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role == Qt::DisplayRole) {
        if (orientation == Qt::Orientation::Vertical) {
            return QVariant(section);
        }
        return QVariant(QString::fromStdString(res.column_name(section)));
    }
    return QVariant();
}

QVariant MyPqxxModel::data(const QModelIndex &index, int role) const {
    if (role == Qt::DisplayRole) {
        return std::visit([&](auto v){
                      auto a2=res[index.row()][index.column()].as<typename decltype(v)::type>();
                      return toQtVariant(a2);
                   }, oidToTypeMap.at(columnTypes[index.column()]));
    }
    return QVariant();
}

void MyPqxxModel::setColumnsTypes() {
    columnTypes.resize(res.columns());
    for (int i = 0; i < res.columns(); ++i) {
        columnTypes[i] = res.column_type(i);
    }
}

