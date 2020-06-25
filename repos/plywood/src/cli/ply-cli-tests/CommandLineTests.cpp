#define CATCH_CONFIG_MAIN
#include <catch.hpp>
#include <ply-cli/CommandLine.h>
#include <ply-cli/Definitions.h>

namespace ply {
namespace cli {

TEST_CASE("Definitions") {
    SECTION("Flags") {
        FlagDefinition flag{"flag1", "description1"};
        flag.setShortName("f1");

        CHECK(flag.name() == "flag1");
        CHECK(flag.description() == "description1");
        CHECK(flag.shortName() == "f1");
    }

    SECTION("Options") {
        OptionDefinition option{"option1", "description1"};
        option.setShortName("o1");

        CHECK(option.name() == "option1");
        CHECK(option.description() == "description1");
        CHECK(option.shortName() == "o1");
    }

    SECTION("Arguments") {
        ArgumentDefinition argument{"argument1", "description1"};
        argument.setRequired(true);

        CHECK(argument.name() == "argument1");
        CHECK(argument.description() == "description1");
        CHECK(argument.isRequired());
    }

    SECTION("Commands") {
        CommandDefinition command1{"command1", "description1"};
        command1.addFlag("flag1", "flagDescription1").setShortName("f1");
        command1.addOption("option1", "optionDescription1").setShortName("o1");
        command1.addArgument("argument1", "argumentDescription1").setRequired(false);
        command1.addSubCommand("subCommand1", "subCommandDescription1");

        CHECK(command1.name() == "command1");
        CHECK(command1.description() == "description1");
    }
}

TEST_CASE("Context") {
    CommandLine cli{"name", "description"};

    SECTION("Find root command") {
        auto context = cli.parse({"tool"});
        CHECK(context.foundLeafCommand());
    }

    SECTION("Find a sub command") {
        cli.addSubCommand("subCommand", "subCommandDescription");
        auto context = cli.parse({"tool", "subCommand"});
        CHECK(context.foundLeafCommand());
    }

    SECTION("Invalid sub command") {
        cli.addSubCommand("subCommand", "subCommandDescription");
        auto context = cli.parse({"tool", "invalid"});
        CHECK(!context.foundLeafCommand());
    }
}

} // namespace cli
} // namespace ply
