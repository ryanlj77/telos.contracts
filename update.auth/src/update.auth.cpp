#include <update.auth/update.auth.hpp>
#include <eosiolib/action.hpp>

void uauth::hello(name executer) {
    print("executer: ", name{executer});
    print("\nself: ", name{_self});
    require_auth(_self);
    print("\nhello executed");
}

void uauth::hello2(name executer) {
    print("executer: ", name{executer});
    print("\nself: ", name{_self});
    require_auth2(executer, "eosio.code"_n);
    print("\nhello executed");
}

void uauth::hello3(name executer) {
    print("executer: ", name{executer});
    print("\nself: ", name{_self});
    print("\nhello executed");
}

void uauth::update(name acc) {
    require_auth(acc);
    require_auth(_self);

    // action(permission_level{_self, "active"_n}, "eosio"_n, "updateauth"_n, 
    //     make_tuple(
    //         "prodname2zi1"_n,
    //         "active"_n,
    //         "owner"_n,
    //         {
                
    //         }
	// )).send();
}

EOSIO_DISPATCH( uauth, (hello)(hello2)(hello3)(update))