#include <update.auth/update.auth.hpp>

void uauth::hello(name executer) {
    print("executer ", name{executer});    
    //    require_auth( _self );
}

EOSIO_DISPATCH( uauth, (hello) )