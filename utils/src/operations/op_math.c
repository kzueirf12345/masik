#include <math.h>

#include "op_math.h"

num_t math_SUM(const num_t first, const num_t second)
{
    return first + second;
}
num_t math_SUB(const num_t first, const num_t second)
{
    return first - second;
}
num_t math_MUL(const num_t first, const num_t second)
{
    return first * second;
}
num_t math_DIV(const num_t first, const num_t second)
{
    if (second == 0)
    {
        fprintf(stderr, "Can't %s\n", __func__);
        return NUM_POISON;
    }
    
    return first / second;
}

num_t math_POW(const num_t first, const num_t second)
{
    return (num_t)powl((long double)first, (long double)second);
}

num_t math_EQ(const num_t first, const num_t second)
{
    return (num_t)(first == second);
}
num_t math_NEQ(const num_t first, const num_t second)
{
    return (num_t)(first != second);
}
num_t math_LESS(const num_t first, const num_t second)
{
    return (num_t)(first < second);
}
num_t math_LESSEQ(const num_t first, const num_t second)
{
    return (num_t)(first <= second);
}
num_t math_GREAT(const num_t first, const num_t second)
{
    return (num_t)(first > second);
}
num_t math_GREATEQ(const num_t first, const num_t second)
{
    return (num_t)(first >= second);
}

//=======================================NOT ARIPHMETIC=========================================

num_t math_LBRAKET(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_RBRAKET(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_PLEASE(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_ASSIGNMENT(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_DECL_ASSIGNMENT(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_DECL_FLAG(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_IF(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_LBODY(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_RBODY(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_COND_LBRAKET(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_COND_RBRAKET(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_WHILE(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_POW_ASSIGNMENT(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_SUM_ASSIGNMENT(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_SUB_ASSIGNMENT(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_MUL_ASSIGNMENT(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_DIV_ASSIGNMENT(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_ELSE(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_FUNC(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_FUNC_LBRAKET(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_FUNC_RBRAKET(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_MAIN(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_ARGS_COMMA(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_CALL_FUNC_LBRAKET(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_CALL_FUNC_RBRAKET(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_RET(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_IN(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}
num_t math_OUT(const num_t first, const num_t second)
{
    (void)first;
    (void)second;
    fprintf(stderr, "%s isn't ariphmetic\n", __func__);
    return NUM_POISON;
}