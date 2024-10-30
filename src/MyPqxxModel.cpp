#include "MyPqxxModel.h"

namespace models {

    MyPqxxModel::MyPqxxModel(QObject *parent) : QAbstractTableModel(parent), MyPxxxModelBase() {
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
            return getData(index, role);
        }
        return QVariant();
    }

    void MyPqxxModel::Reset() {
        beginResetModel();
        MyPxxxModelBase::reset();
        endResetModel();
    }


    void MyPxxxModelBase::setColumnsTypes() {
        columnTypes.resize(res.columns());
        columnNames.resize(res.columns());
        for (int i = 0; i < res.columns(); ++i) {
            columnTypes[i] = res.column_type(i);
            columnNames[i] = res.column_name(i);
        }
    }

    bool MyPxxxModelBase::performConnection(myConnString &cstring) {
        this->connection_ = connectIfPossible(cstring.c_str()).value_or(nullptr);
        good = checkConnection();
        return good;
    }

    QVariant MyPxxxModelBase::getData(const QModelIndex &index, int role) const {
        if (columnTypes.contains(index.column())) {
            if (res[index.row()][index.column()].is_null()) {
                return QVariant();
            }
            return toQtVariant(res[index.row()][index.column()].as<std::string>());
        }
        return std::visit([&](auto v) {
            if (res[index.row()][index.column()].is_null()) {
                return QVariant();
            }
            return toQtVariant(res[index.row()][index.column()].as<typename decltype(v)::type>());
        }, oidToTypeMap.at(columnTypes[index.column()]));
    }

    bool MyPxxxModelBase::checkConnection() {
        good = db_services::checkConnection(connection_);
        return good;
    }


}