#include <Functions/IFunction.h>
#include <Functions/FunctionHelpers.h>
#include <Functions/FunctionFactory.h>
#include <DataTypes/DataTypesNumber.h>
#include <DataTypes/DataTypeNullable.h>
#include <Core/ColumnNumbers.h>
#include <Columns/ColumnNullable.h>


namespace DB
{
namespace
{

/// Implements the function nullIf which takes 2 arguments and returns
/// NULL if both arguments have the same value. Otherwise it returns the
/// value of the first argument.
class FunctionNullIf : public IFunction, WithContext
{
public:
    static constexpr auto name = "nullIf";

    static FunctionPtr create(ContextPtr context_)
    {
        return std::make_shared<FunctionNullIf>(context_);
    }

    explicit FunctionNullIf(ContextPtr context_) : WithContext(context_) {}

    std::string getName() const override
    {
        return name;
    }

    size_t getNumberOfArguments() const override { return 2; }
    bool useDefaultImplementationForNulls() const override { return false; }
    bool useDefaultImplementationForConstants() const override { return true; }
    bool isSuitableForShortCircuitArgumentsExecution(const DataTypesWithConstInfo & /*arguments*/) const override { return false; }

    DataTypePtr getReturnTypeImpl(const DataTypes & arguments) const override
    {
        return makeNullable(arguments[0]);
    }

    ColumnPtr executeImpl(const ColumnsWithTypeAndName & arguments, const DataTypePtr & result_type, size_t input_rows_count) const override
    {
        /// nullIf(col1, col2) == if(col1 = col2, NULL, col1)

        auto equals_func = FunctionFactory::instance().get("equals", getContext())->build(arguments);
        auto eq_res = equals_func->execute(arguments, equals_func->getResultType(), input_rows_count);

        ColumnsWithTypeAndName if_columns
        {
            {eq_res, equals_func->getResultType(), ""},
            {result_type->createColumnConstWithDefaultValue(input_rows_count), result_type, "NULL"},
            arguments[0],
        };

        auto func_if = FunctionFactory::instance().get("if", getContext())->build(if_columns);
        auto if_res = func_if->execute(if_columns, result_type, input_rows_count);

        return makeNullable(if_res);
    }
};

}

REGISTER_FUNCTION(NullIf)
{
    factory.registerFunction<FunctionNullIf>({}, FunctionFactory::CaseInsensitive);
}

}
