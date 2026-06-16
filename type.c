TypeAndValue tav_lvalue(Type *type) {
    TypeAndValue result = (TypeAndValue){ .type = type, .mode = ADRESSING_VARIABLE };
    return result;
}
TypeAndValue tav_rvalue(Type *type) {
    TypeAndValue result = (TypeAndValue){ .type = type, .mode = ADRESSING_VALUE };
    return result;
}
TypeAndValue tav_const(Type *type, Value value) {
    TypeAndValue result = (TypeAndValue){ .type = type, .mode = ADRESSING_CONSTANT, .value = value };
    return result;
}
// TypeAndValue tav_novalue(void) {
//     TypeAndValue result = (TypeAndValue){ type =  .mode = ADRESSING_NO_VALUE };
//     return result;
// }