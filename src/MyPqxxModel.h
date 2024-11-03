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

/// models namespace
namespace models {
    /**
     * Type for postgresql OID storage
     */
    using OID = uint32_t;

    /**
     * Variant type for runtime type evaluation
     */
    using returnType = std::variant <std::type_identity<int64_t>,
    std::type_identity<double>,
    std::type_identity<std::string>,
    std::type_identity<pqxx::binarystring>>;

    /**
     *  Map from postgresql OID to some types
     */
    static const std::unordered_map <OID, returnType> oidToTypeMap = {
            {20,   std::type_identity < int64_t > {}},
            {701,  std::type_identity < double > {}},
            {25,   std::type_identity < std::string > {}},
            {17,   std::type_identity < pqxx::binarystring > {}},
            {1700, std::type_identity < double > {}}
    };

    /**
     * To qtvariant converter
     * @tparam ReturnType
     * @param val
     * @return
     */
    template<typename ReturnType>
    QVariant
    toQtVariant(const ReturnType &val) {
        return QVariant();
    }

    template<>
    QVariant inline toQtVariant(const pqxx::binarystring &val) {
        return QVariant(QString::fromStdString(val.str()));
    }

    template<>
    QVariant inline toQtVariant(const std::string_view &val) {
        return QVariant(QString::fromStdString(val.data()));
    }

    template<>
    QVariant inline toQtVariant(const std::string &val) {
        return QVariant(QString::fromStdString(val));
    }

    template<>
    QVariant inline toQtVariant(const double &val) {
        return val;
    }


    QVariant inline toQtVariant(const int64_t &val) {
        return static_cast<qint64>(val);
    }

    using namespace db_services;

    /**
     * @brief Basic class for database pqxx::result representation
     */
    class MyPxxxModelBase {
    public:
        /**
         * Connects to database using given connection string
         * @param cstring
         */
        bool performConnection(myConnString &cstring);

        /**
         * Checks connection  status
         */
        bool checkConnection();

        virtual void getData()=0;



        /**
         * Checks models emptiness
         * @return
         */
        bool isEmpty() const {
            return isEmpty_;
        }

    protected:

        /**
         * @brief Returns properly casted data
         * Uses postgres column OID to perform cast to suitable type that will be wrapped in QVariant
         * @details This function is used in order for elements in inheriting models
         * @details to be able to get native data type for better sorting etc.
         * @param index
         * @param role
         * @return
         */
        QVariant getData(const QModelIndex &index, int role) const;

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
                                  Args &&... args);

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
                             Args &&... args);

        void setColumnsTypes();
        /**
         * Resets result
         */
        void reset() {
            res = resType();
            isEmpty_ = true;
        }

        resType res;
        conPtr connection_;
        QList <OID> columnTypes;
        QList <QString> columnNames;
        bool good;
        bool isEmpty_;
    };


    /**
     * @brief Table model for pqxx::result @see models::MyPxxxModelBase
     */
    class MyPqxxModel : public QAbstractTableModel, public MyPxxxModelBase {
        Q_OBJECT
    public:

        MyPqxxModel(QObject *parent = nullptr);

        int rowCount(const QModelIndex &parent) const override {
            return res.size();
        }

        int columnCount(const QModelIndex &parent) const override {
            return res.columns();
        }

        /**
         * Shows headers
         * @param section
         * @param orientation
         * @param role
         * @return
         */
        QVariant headerData(int section, Qt::Orientation orientation,
                            int role = Qt::DisplayRole) const override;

        QVariant data(const QModelIndex &index, int role) const override;

        /**
         * Resets result and updates model
         */
        void Reset();
    };

    /**
     * @brief Pqxx model that retrieves database deduplication characteristic @see db_services::getDedupCharacteristics
     */
    class DeduplicationCharacteristicsModel : public MyPqxxModel {
    public:
        DeduplicationCharacteristicsModel(QObject *parent = nullptr) : MyPqxxModel(parent) {}

        /**
         * Retrieves specified data for model
         */
        void getData() override {
            beginResetModel();
            this->executeInTransaction(&db_services::getDedupCharacteristics);
            endResetModel();
        }
    };


    /**
     * @brief Sort filter proxy model used for searching entry names
     */
    class MySortFilterProxyModel : public QSortFilterProxyModel {
        Q_OBJECT
    public:
        MySortFilterProxyModel(QObject *parent = nullptr)
                : QSortFilterProxyModel(parent) {
        }


    protected:
        /**
         * Filter files whose path contain sourceRow
         * @param sourceRow
         * @param sourceParent
         */
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
            QModelIndex index0 = sourceModel()->index(sourceRow, 0, sourceParent);


            return (sourceModel()->data(index0).toString().contains(filterRegularExpression()));
        }
    };


    ///Sort filter model that filters out all rows with NULL values
    class NotNullFilterProxyModel : public QSortFilterProxyModel {
        Q_OBJECT

    public:
        NotNullFilterProxyModel(QObject *parent = nullptr)
                : QSortFilterProxyModel(parent) {
        }


    protected:
        /**
         * Filters out rows with empty values
         * @param sourceRow
         * @param sourceParent
         */
        bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override {
            for (size_t i = 0; i < sourceModel()->columnCount(); ++i) {
                if (sourceModel()->data(sourceModel()->index(sourceRow, i, sourceParent)).isNull()) {
                    return false;
                }
            }
            return true;
        }
    };

    ///Tbale view that can be deselected
    class DeselectableTableView : public QTableView {
    public:
        DeselectableTableView(QWidget *parent) : QTableView(parent) {}

        virtual ~DeselectableTableView() {}

    private:
        /**
         * Mouse press event handler
         * @param event
         */
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


    template<typename ResType1, typename... Args>
    void
    MyPxxxModelBase::executeInTransaction(ResType1 (*call)(db_services::trasnactionType &, Args...), Args &&... args) {
        if (!checkConnection()) {
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

    template<typename ResType1, typename... Args>
    void MyPxxxModelBase::executeInTransaction(const std::function<ResType1(trasnactionType &, Args...)> &call,
                                               Args &&... args) {
        if (!checkConnection()) {
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

};

#endif //DATA_DEDUPLICATION_SERVICE_MYPQXXMODEL_H
