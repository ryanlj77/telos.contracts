#include <update.auth/update.auth.hpp>
#include <eosiolib/action.hpp>


void uauth::hello(name executer) {
    print("executer: ", name{executer});
    print("\nself: ", name{_self});
    require_auth(_self);
    print("\nhello executed");
}

void uauth::setperm() {
    require_auth(_self);
    //TODO: send inline updateauth action to create a new permission for testaccounta (the owner of this contract.)
    action(
        permission_level{get_self(), "active"_n}, 
        "eosio"_n, 
        "updateauth"_n, 
        std::make_tuple(
            get_self(),
            "newperm"_n,
            "active"_n,
            authority { 
                1, 
                std::vector<key_weight> {},
                std::vector<permission_level_weight> {
                    permission_level_weight{ permission_level{ get_self(), "newperm"_n}, 1 }
                },
                std::vector<wait_weight> {}
            }
	    )
    ).send();
}

EOSIO_DISPATCH( uauth, (hello)(setperm))
