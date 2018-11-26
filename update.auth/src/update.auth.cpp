#include <update.auth/update.auth.hpp>
#include <eosiolib/action.hpp>


void uauth::hello(name executer) {
    print("executer: ", name{executer});
    print("\nself: ", name{_self});
    require_auth(_self);
    print("\nhello executed");
}

void uauth::setperm(std::string accperm, std::string newperm ) {
    require_auth(_self);
    //TODO: send inline updateauth action to create a new permission for testaccounta (the owner of this contract.)
    action(
        permission_level{get_self(), name(accperm)}, 
        "eosio"_n, 
        "updateauth"_n, 
        std::make_tuple(
            get_self(),
            name(newperm),
            name(accperm),
            authority { 
                1, 
                std::vector<key_weight> {},
                std::vector<permission_level_weight> {
                    permission_level_weight{ permission_level{ get_self(), "active"_n}, 1 }
                },
                std::vector<wait_weight> {}
            }
	    )
    ).send();
}

void uauth::levelperm(std::string perm){
    require_auth2(_self.value, name(perm).value);
    print("\nexecuted :)");
}



EOSIO_DISPATCH( uauth, (hello)(setperm)(levelperm))
