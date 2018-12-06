/**
 * Supports chain storage of proxies
 * 
 * @author Marlon Williams
 * @copyright defined in telos/LICENSE.txt
 */

#include <telos.proxy/telos.proxy.hpp>

tlsproxyinfo::tlsproxyinfo(name self, name code, datastream<const char*> ds) : 
contract(self, code, ds), 
proxies(self, self.value)
{

}

tlsproxyinfo::~tlsproxyinfo() {
}

/**
 *  Set proxy info
 *
 *  @param proxy - the proxy account name
 *  @param name - human readable name
 *  @param slogan - a short description
 *  @param philosophy - voting philosophy
 *  @param background - optional. who is the proxy?
 *  @param website - optional. url to website
 *  @param logo_256 - optional. url to an image with the size of 256 x 256 px
 *  @param telegram - optional. telegram account name
 *  @param steemit - optional. steemit account name
 *  @param twitter - optional. twitter account name
 *  @param wechat - optional. wechat account name
 */
void tlsproxyinfo::set(const name proxy, std::string name, std::string slogan, std::string philosophy, std::string background, std::string website, std::string logo_256, std::string telegram, std::string steemit, std::string twitter, std::string wechat, std::string reserved_1, std::string reserved_2, std::string reserved_3) {
    // Validate input
    eosio_assert(!name.empty(), "name required");
    eosio_assert(name.length() <= 256, "name too long");

    eosio_assert(slogan.length() <= 256, "slogan too long");

    eosio_assert(philosophy.length() <= 256, "philosophy too long");

    eosio_assert(background.length() <= 256, "background too long");

    eosio_assert(website.length() <= 256, "website too long");
    if (!website.empty()) {
        eosio_assert(website.substr(0, 4) == "http", "website should begin with http");
    }

    eosio_assert(logo_256.length() <= 256, "logo_256 too long");
    if (!logo_256.empty()) {
        eosio_assert(logo_256.substr(0, 4) == "http", "logo_256 should begin with http");
    }

    eosio_assert(telegram.length() <= 64, "telegram too long");
    eosio_assert(steemit.length() <= 64, "steemit too long");
    eosio_assert(twitter.length() <= 64, "twitter too long");
    eosio_assert(wechat.length() <= 64, "wechat too long");

    // Require auth from the proxy account
    require_auth(proxy);

    // Check if exists
    auto current = proxies.find(proxy.value);

    // Update
    if (current != proxies.end()) {
        proxies.modify(current, proxy, [&]( auto& i ) {
            i.owner = proxy;
            i.name = name;
            i.website = website;
            i.slogan = slogan;
            i.philosophy = philosophy;
            i.background = background;
            i.logo_256 = logo_256;
            i.telegram = telegram;
            i.steemit = steemit;
            i.twitter = twitter;
            i.wechat = wechat;
            i.reserved_1 = reserved_1;
            i.reserved_2 = reserved_2;
            i.reserved_3 = reserved_3;
        });

    // Insert
    } else {
        proxies.emplace(proxy, [&]( auto& i ) {
            i.owner = proxy;
            i.name = name;
            i.website = website;
            i.slogan = slogan;
            i.philosophy = philosophy;
            i.background = background;
            i.logo_256 = logo_256;
            i.telegram = telegram;
            i.steemit = steemit;
            i.twitter = twitter;
            i.wechat = wechat;
            i.reserved_1 = reserved_1;
            i.reserved_2 = reserved_2;
            i.reserved_3 = reserved_3;
        });
    }
}

/**
 * Remove all proxy info.
 *g
 * @param proxy - the proxy account name.
 */
void tlsproxyinfo::remove(const name proxy) {
    // Require auth from the proxy account
    require_auth(proxy);

    // Delete record
    auto lookup = proxies.find(proxy.value);
    if (lookup != proxies.end()) {
        proxies.erase(lookup);
    }
}

EOSIO_DISPATCH(tlsproxyinfo, (set)(remove))