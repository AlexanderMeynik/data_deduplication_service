#ifndef SOURCE_SERVICE_CLASSWRAPPERS_H
#define SOURCE_SERVICE_CLASSWRAPPERS_H
#include <pqxx/pqxx>

class connectionWrapper
{
    connectionWrapper():inner_(){};
    virtual bool is_open() const noexcept
    {
        return inner_.is_open();
    };

    explicit connectionWrapper(pqxx::zview options) : inner_(options.c_str())
    //todo divide into impl and constructor
    {

    }

    connectionWrapper(connectionWrapper &&rhs):inner_(std::move(rhs.inner_))
    {
        //todo divide into impl and constructor

    }


private:
    pqxx::connection inner_;
};
using pqxx::isolation_level,pqxx::write_policy;
template<typename connectionType=pqxx::connection,typename ResultType=pqxx::result,isolation_level ISOLATION = isolation_level::read_committed,
        write_policy READWRITE = write_policy::read_write>
class transactionWrapper
{
    explicit transactionWrapper(connectionType &cx) : inner_(cx)
    {
        //todo divide into impl and constructor
    }

    virtual ResultType exec(std::string_view query)
    {
        return inner_.exec(query);
    }
    virtual void commit(){
        inner_.commit();
    };


    template<typename TYPE> TYPE query_value(pqxx::zview query)//todo cannot be used as virtual
    {
        return inner_.template query_value<TYPE>(query);
    }


    virtual ResultType exec(std::string_view query, pqxx::params parms)
    {
        return inner_.exec(query, parms);
    }
    //todo 253 streams and how to handle them

private:
    pqxx::transaction<ISOLATION,READWRITE> inner_;
};




template<typename connectionType=pqxx::connection,typename ResultType=pqxx::result>
class notransactionWrapper
{
    explicit notransactionWrapper(connectionType &cx) : inner_(cx)
    {
        //todo divide into impl and constructor
    }

    virtual ResultType exec(std::string_view query)
    {
        return inner_.exec(query);
    }
    virtual void commit(){
        inner_.commit();
    };


    template<typename TYPE> TYPE query_value(pqxx::zview query)//todo cannot be used as virtual
    {
        return inner_.template query_value<TYPE>(query);
    }


    virtual ResultType exec(std::string_view query, pqxx::params parms)
    {
        return inner_.exec(query, parms);
    }
    virtual void abort()
    {
        inner_.abort();
    }
    //todo this one is the same as transction
    //todo 2 esc function must be removed

private:
    pqxx::nontransaction inner_;
};

class resultWrapper
{
    using reference = pqxx::field;
    using size_type = pqxx::row_size_type;
    resultWrapper() noexcept : inner_()
    {}

    virtual resultWrapper &operator=(resultWrapper &&rhs) noexcept = default;
    virtual pqxx::row one_row() const
    {
        return inner_.one_row();
    };
    virtual pqxx::result no_rows() const
    {
        return inner_.no_rows();
    };

    //todo 502 begin end const iterators
    //todo have to do something with fields(503)

private:
    pqxx::result inner_;
};
#endif //SOURCE_SERVICE_CLASSWRAPPERS_H
