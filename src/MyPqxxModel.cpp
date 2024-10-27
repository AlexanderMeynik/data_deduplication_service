#include "MyPqxxModel.h"


MyPqxxModel::MyPqxxModel(QObject *parent) : QAbstractTableModel(parent) {
    res = pqxx::result();
    isEmpty_ = true;
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
        return std::visit([&](auto v) {
            if(res[index.row()][index.column()].is_null())
            {
                return QVariant();
            }
            auto a2 = res[index.row()][index.column()].as<typename decltype(v)::type>();
            return toQtVariant(a2);
        }, oidToTypeMap.at(columnTypes[index.column()]));
    }
    return QVariant();
}

void MyPqxxModel::setColumnsTypes() {
    columnTypes.resize(res.columns());
    columnNames.resize(res.columns());
    for (int i = 0; i < res.columns(); ++i) {
        columnTypes[i] = res.column_type(i);
        columnNames[i] = res.column_name(i);
    }
}

bool MyPqxxModel::performConnection(myConnString &cstring) {
    this->connection_ = connectIfPossible(cstring.c_str()).value_or(nullptr);
    good = db_services::checkConnection(this->connection_);
    return good;
}

