#ifndef DATA_DEDUPLICATION_SERVICE_MYPQXXMODEL_H
#define DATA_DEDUPLICATION_SERVICE_MYPQXXMODEL_H


#include <variant>
#include <unordered_map>

#include <QAbstractTableModel>
#include <QSortFilterProxyModel>
#include <pqxx/pqxx>

#include <dbCommon.h>
#include <QDate>
#include <QMouseEvent>
#include <QTableView>
/**
 * Type for postgresql OID storage
 */
using OID = uint32_t;

/**
 * Variant type for runtime type evaluation
 */
using returnType = std::variant<std::type_identity<int64_t>,
        std::type_identity<double>,
        std::type_identity<std::string>,
        std::type_identity<pqxx::binarystring>>;
/**
 *  Map from postgresql OID to some types
 */
static const std::unordered_map<OID, returnType> oidToTypeMap = {
        {20,  std::type_identity<int64_t>{}},
        {701, std::type_identity<double>{}},
        {25,  std::type_identity<std::string>{}},
        {17,  std::type_identity<pqxx::binarystring>{}},
        {1700, std::type_identity<double>{}}
};


template<typename Ret>
QVariant
toQtVariant(Ret &val) {
    return QVariant();
}

template<>
QVariant inline toQtVariant(pqxx::binarystring &val) {
    return QVariant(QString::fromStdString(val.str()));
}

template<>
QVariant inline toQtVariant(std::string_view &val) {
    return QVariant(QString::fromStdString(val.data()));
}

template<>
QVariant inline toQtVariant(std::string &val) {
    return QVariant(QString::fromStdString(val));
}

template<>
QVariant inline toQtVariant(double &val) {
    return val;
}


QVariant inline toQtVariant(int64_t &val) {
    return static_cast<qint64>(val);
}

using namespace db_services;

/**
 * Model class for libpqxx result
 */
class MyPxxxModelBase
{
public:
    bool performConnection(myConnString &cstring);
    bool checkConnection()
    {
        good= db_services::checkConnection(connection_);
        return good;
    }

    QVariant getData(const QModelIndex &index, int role) const
    {
        return std::visit([&](auto v) {
            if(res[index.row()][index.column()].is_null())
            {
                return QVariant();
            }
            auto a2 = res[index.row()][index.column()].as<typename decltype(v)::type>();
            return toQtVariant(a2);
        }, oidToTypeMap.at(columnTypes[index.column()]));
    }
    /**
     * @ref db_services::executeInTransaction() "executeInTransaction()"
     * @tparam ResType1
     * @tparam Args
     * @param call
     * @param args
     */
    template<typename ResType1, typename ... Args>
    void executeInTransaction(ResType1
                              (*call)(db_services::trasnactionType &, Args ...),
                              Args &&... args) {
        if(!checkConnection())
        {
            res = pqxx::result();
            return;
        }
        try {
            auto ss = db_services::executeInTransaction(connection_, call, std::forward<Args>(args)...);
            if (!ss.has_value()) {
                res = pqxx::result();
            }
            res = ss.value();
        }
        catch (const pqxx::sql_error &e) {
            res = pqxx::result();
            return;
        }
        catch (const std::exception &e) {
            res = pqxx::result();
            return;
        }

        setColumnsTypes();
        isEmpty_ = false;
    }

    /**
     * @ref db_services::executeInTransaction(conPtr &conn_,
     * ResultType (*call)(trasnactionType &, Args ...), Args &&... args) "executeInTransaction()"
     * @tparam ResType1
     * @tparam Args
     * @param call
     * @param args
     */
    template<typename ResType1, typename ... Args>
    void
    executeInTransaction(const std::function<ResType1(db_services::trasnactionType &, Args ...)> &call,
                         Args &&... args) {

        if(!checkConnection())
        {
            res = pqxx::result();
            return;
        }
        try {
            auto ss = db_services::executeInTransaction(connection_, call, std::forward<Args>(args)...);
            if (!ss.has_value()) {
                res = pqxx::result();
            }
            res = ss.value();
        }
        catch (const pqxx::sql_error &e) {
            res = pqxx::result();
            return;
        }
        catch (const std::exception &e) {
            res = pqxx::result();
            return;
        }

        setColumnsTypes();
        isEmpty_ = false;
    }

    void reset() {
        res = resType();
        isEmpty_ = true;
    }

    bool connected() const {
        return good;
    }

    bool isEmpty() {
        return isEmpty_;
    }
protected:
    void setColumnsTypes();

    resType res;

    conPtr connection_;
    QList<OID> columnTypes;
    QList<QString> columnNames;
    bool good;
    bool isEmpty_;
};


class MyPqxxModel : public QAbstractTableModel, public MyPxxxModelBase{
Q_OBJECT
public:

    MyPqxxModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override {
        return res.size();
    }

    int columnCount(const QModelIndex &parent) const override {
        return res.columns();
    }

    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QVariant data(const QModelIndex &index, int role) const override;

    void reset() {
        beginResetModel();
        MyPxxxModelBase::reset();
        endResetModel();
    }
};

class MainTableModel : public MyPqxxModel {
public:
    MainTableModel(QObject *parent = nullptr) : MyPqxxModel(parent) {}

    void getData() {
        beginResetModel();
        this->executeInTransaction(&db_services::getDedupCharacteristics);
        endResetModel();
    }
};


class MySortFilterProxyModel : public QSortFilterProxyModel {
Q_OBJECT

public:
    MySortFilterProxyModel(QObject *parent = nullptr)
            : QSortFilterProxyModel(parent) {
    }


protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
        QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);


        return (sourceModel()->data(index0).toString().contains(filterRegularExpression()));
    }
};


class NotNullFilterProxyModel : public QSortFilterProxyModel {
Q_OBJECT

public:
    NotNullFilterProxyModel(QObject *parent = nullptr)
            : QSortFilterProxyModel(parent) {
    }


protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
        for (size_t i = 0; i < sourceModel()->columnCount(); ++i) {
            if(sourceModel()->data(sourceModel()->index(sourceRow, i, sourceParent)).isNull())
            {
                return false;
            }
        }
        return true;
    }
};



class DeselectableTreeView : public QTableView {
public:
    DeselectableTreeView(QWidget *parent) : QTableView(parent) {}

    virtual ~DeselectableTreeView() {}

private:
    virtual void mousePressEvent(QMouseEvent *event) {

        QModelIndex item = indexAt(event->pos());

        if (item.isValid()) {
            QTableView::mousePressEvent(event);
        } else {
            clearSelection();
            const QModelIndex index;
            selectionModel()->setCurrentIndex(index, QItemSelectionModel::Select);
        }
    }

};


#endif //DATA_DEDUPLICATION_SERVICE_MYPQXXMODEL_H
