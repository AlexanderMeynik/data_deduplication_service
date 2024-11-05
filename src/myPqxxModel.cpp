#include "myPqxxModel.h"

namespace models {

    myPqxxModel::myPqxxModel(QObject *parent) : QAbstractTableModel(parent), myPxxxModelBase() {
        res = pqxx::result();
        isEmpty_ = true;
    }

    QVariant myPqxxModel::headerData(int section, Qt::Orientation orientation, int role) const {
        if (role == Qt::DisplayRole) {
            if (orientation == Qt::Orientation::Vertical) {
                return {section};
            }
            return {(QString::fromStdString(res.column_name(section)))};
        }
        return {};
    }

    QVariant myPqxxModel::data(const QModelIndex &index, int role) const {
        if (role == Qt::DisplayRole) {
            return getData(index, role);
        }
        return {};
    }

    void myPqxxModel::Reset() {
        beginResetModel();
        myPxxxModelBase::reset();
        endResetModel();
    }


    void myPxxxModelBase::setColumnsTypes() {
        columnTypes.resize(res.columns());
        columnNames.resize(res.columns());
        for (int i = 0; i < res.columns(); ++i) {
            columnTypes[i] = res.column_type(i);
            columnNames[i] = res.column_name(i);
        }
    }

    bool myPxxxModelBase::performConnection(myConnString &cstring) {
        this->connection = connectIfPossible(cstring.c_str()).value_or(nullptr);
        good = checkConnection();
        return good;
    }

    QVariant myPxxxModelBase::getData(const QModelIndex &index, int role) const {
        if (columnTypes.contains(index.column())) {
            if (res[index.row()][index.column()].is_null()) {
                return {};
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

    bool myPxxxModelBase::checkConnection() {
        good = db_services::checkConnection(connection);
        return good;
    }


}