#ifndef SOURCE_SERVICE_CLASSWRAPPERS_H
#define SOURCE_SERVICE_CLASSWRAPPERS_H
#include <pqxx/pqxx>

class connectionWrapper
{

    connectionWrapper() {
        empty_contr_impl();
    }

    void empty_contr_impl(){
        inner_=pqxx::connection();
    };
    virtual bool is_open() const noexcept
    {
        return inner_.is_open();
    };

    explicit connectionWrapper(pqxx::zview options)
    {
        connection_cont_impl(options);
    }
    void connection_cont_impl(pqxx::zview options)
    {
        inner_=pqxx::connection{options.c_str()};
    }

    connectionWrapper(connectionWrapper &&rhs) {
        connn_contr_impl(std::move(rhs));
    }
    virtual void connn_contr_impl(connectionWrapper &&rhs)
    {
        inner_=std::move(rhs.inner_);
    }


private:
    pqxx::connection inner_;
};
using pqxx::isolation_level,pqxx::write_policy;
template<typename connectionType=pqxx::connection,typename ResultType=pqxx::result,isolation_level ISOLATION = isolation_level::read_committed,
        write_policy READWRITE = write_policy::read_write>
class transactionWrapper
{
    transactionWrapper(connectionType &cx, std::string_view tname = "")
    {
        trans_constr_impl(cx,tname);
    }
    virtual void trans_constr_impl(connectionType &cx)
    {
        inner_={cx,cx};
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


    notransactionWrapper(connectionType &cx, std::string_view tname = "")
    {
        nonTrans_constr_impl(cx,tname);
    }
    virtual void nonTrans_constr_impl(connectionType &cx)
    {
        inner_={cx,cx};
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

private:
    pqxx::nontransaction inner_;
};

class resultWrapper
{
    using reference = pqxx::field;
    using size_type = pqxx::row_size_type;
    resultWrapper() noexcept
    {
        const_impl();
    }

    virtual void const_impl()noexcept
    {
        inner_=pqxx::result();
    }

    virtual resultWrapper &operator=(resultWrapper &&rhs) noexcept = default;
    virtual pqxx::row one_row() const
    {
        return inner_.one_row();
    };
    virtual pqxx::result no_rows() const
    {
        return inner_.no_rows();
    };

    virtual pqxx::const_result_iterator begin()
    {
        return inner_.begin();
    }

    virtual pqxx::const_result_iterator end()
    {
        return inner_.end();
    }
    //todo have to do something with fields(503)

private:
    pqxx::result inner_;
};
#endif //SOURCE_SERVICE_CLASSWRAPPERS_H
