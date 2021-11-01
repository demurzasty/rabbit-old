#pragma once

#include <string>

namespace rb {
    struct identity {
        std::string name;

		template<typename Visitor>
		static void visit(Visitor& visitor, identity& identity) {
			visitor("name", identity.name);
		}
    };
}
