#include <boost/test/unit_test.hpp>
#include <eosio/testing/tester.hpp>
#include <eosio/chain/abi_serializer.hpp>
#include <eosio/chain/wast_to_wasm.hpp>

#include <Runtime/Runtime.h>

#include <fc/variant_object.hpp>
#include "contracts.hpp"
#include "test_symbol.hpp"
#include "eosio.trail_tester.hpp"

using namespace eosio::testing;
using namespace eosio;
using namespace eosio::chain;
using namespace eosio::testing;
using namespace fc;
using namespace std;

using mvo = fc::mutable_variant_object;

BOOST_AUTO_TEST_SUITE(eosio_trail_tests)

BOOST_FIXTURE_TEST_CASE( reg_voters, eosio_trail_tester ) try {
	std::cout << "test_voters.size(): " << test_voters.size() << std::endl;
	for (int i = 0; i < test_voters.size(); i++) {
		std::cout << "regvoter for: " << test_voters[i].to_string() << std::endl;
		regvoter(test_voters[i].value);
		produce_blocks();
		auto voter_info = get_voter(test_voters[i], symbol(4, "VOTE").to_symbol_code());
		
		REQUIRE_MATCHING_OBJECT(voter_info, mvo()
			("owner", test_voters[i].to_string())
			("tokens", "0.0000 VOTE")
		);

		std::cout << "unregvoter for: " << test_voters[i].to_string() << std::endl;
		unregvoter(test_voters[i].value);
		produce_blocks();
		voter_info = get_voter(test_voters[i], symbol(4, "VOTE").to_symbol_code());
		BOOST_REQUIRE_EQUAL(true, voter_info.is_null());
	}
} FC_LOG_AND_RETHROW()

//TODO: check for voter already exists assertion msg
BOOST_FIXTURE_TEST_CASE( vote_levies, eosio_trail_tester ) try {
	create_accounts({N(levytest1), N(levytest2)});
	produce_blocks( 2 );
	//transfer 1
	transfer(N(eosio), N(levytest1), asset::from_string("1000.0000 TLOS"), "Monopoly Money");
	uint32_t initial_transfer_time = now();
	//from levy 1
	auto levy_info = get_vote_counter_bal(N(eosio), symbol(4, "VOTE").to_symbol_code());
	REQUIRE_MATCHING_OBJECT(levy_info, mvo()
			("owner", "eosio")
			("decayable_cb", "0.0000 VOTE")
			("persistent_cb", "0.0000 VOTE")
			("last_decay", initial_transfer_time)
		);

	//to levy 1
	levy_info = get_vote_counter_bal(N(levytest1), symbol(4, "VOTE").to_symbol_code());
	std::cout << "decayable_cb: " << levy_info["decayable_cb"].as_string() << std::endl;
	std::cout << "persistent_cb: " << levy_info["persistent_cb"].as_string() << std::endl;
	REQUIRE_MATCHING_OBJECT(levy_info, mvo()
			("owner", "levytest1")
			("decayable_cb", "1000.0000 VOTE")
			("persistent_cb", "0.0000 VOTE")
			("last_decay", initial_transfer_time)
		);

	//transfer 2
	transfer(N(levytest1), N(levytest2), asset::from_string("200.0000 TLOS"), "Monopoly Money");

	//from 2 
	levy_info = get_vote_counter_bal(N(levytest1), symbol(4, "VOTE").to_symbol_code());
	REQUIRE_MATCHING_OBJECT(levy_info, mvo()
			("owner", "levytest1")
			("decayable_cb", "800.0000 VOTE")
			("persistent_cb", "0.0000 VOTE")
			("last_decay", initial_transfer_time)
		);

	//to 2
	levy_info = get_vote_counter_bal(N(levytest2), symbol(4, "VOTE").to_symbol_code());
	REQUIRE_MATCHING_OBJECT(levy_info, mvo()
			("owner", "levytest2")
			("decayable_cb", "200.0000 VOTE")
			("persistent_cb", "0.0000 VOTE")
			("last_decay", initial_transfer_time)
		);

	produce_blocks( 2 );
	//tranfer 3
	transfer(N(levytest2), N(levytest1), asset::from_string("100.0000 TLOS"), "Monopoly Money");

	//from 3 
	levy_info = get_vote_counter_bal(N(levytest2), symbol(4, "VOTE").to_symbol_code());
	REQUIRE_MATCHING_OBJECT(levy_info, mvo()
			("owner", "levytest2")
			("decayable_cb", "100.0000 VOTE")
			("persistent_cb", "0.0000 VOTE")
			("last_decay", initial_transfer_time)
		);

	//to 2
	levy_info = get_vote_counter_bal(N(levytest1), symbol(4, "VOTE").to_symbol_code());
	REQUIRE_MATCHING_OBJECT(levy_info, mvo()
			("owner", "levytest1")
			("decayable_cb", "900.0000 VOTE")
			("persistent_cb", "0.0000 VOTE")
			("last_decay", initial_transfer_time)
		);

	create_accounts({N(levytest3)});
	//transfer 3
	transfer(N(levytest1), N(levytest3), asset::from_string("100.0000 TLOS"), "Monopoly Money");
	levy_info = get_vote_counter_bal(N(levytest1), symbol(4, "VOTE").to_symbol_code());
	REQUIRE_MATCHING_OBJECT(levy_info, mvo()
			("owner", "levytest1")
			("decayable_cb", "800.0000 VOTE")
			("persistent_cb", "0.0000 VOTE")
			("last_decay", initial_transfer_time)
		);

	levy_info = get_vote_counter_bal(N(levytest3), symbol(4, "VOTE").to_symbol_code());
	REQUIRE_MATCHING_OBJECT(levy_info, mvo()
			("owner", "levytest3")
			("decayable_cb", "100.0000 VOTE")
			("persistent_cb", "0.0000 VOTE")
			("last_decay", now())
		);
} FC_LOG_AND_RETHROW()

//TODO: check minimum lock period assertion msg
//TODO: check maximum lock period assertion msg
//TODO: check for positive VOTE quantity assertion msg
//TODO: check for non-registered voter assertion msg
BOOST_FIXTURE_TEST_CASE( mirror_cast, eosio_trail_tester ) try {
	fc::variant voter_info = mvo();
	uint32_t future = 0;
	for (int i = 0; i < test_voters.size(); i++) {
		regvoter(test_voters[i].value);
		std::cout << "regvoter for: " << test_voters[i].to_string() << std::endl;

		// asset balance = get_currency_balance(N(eosio.token), symbol(4, "TLOS"), test_voters[i].value);
		// std::cout << "test_voter " << test_voters[i].to_string() << ": " << balance.get_amount() << std::endl;
		mirrorcast(test_voters[i].value, symbol(4, "VOTE"));
		voter_info = get_voter(test_voters[i], symbol(4, "VOTE").to_symbol_code());
		REQUIRE_MATCHING_OBJECT(voter_info, mvo()
			("owner", test_voters[i].to_string())
			("tokens", "200.0000 VOTE")
		);
		produce_blocks( 2 );
	}
	//Transfer to a new account
	std::cout << "creating votedecay..." << std::endl;
	create_accounts({N(votedecay)});
	std::cout << "transfering to votedecay account" << std::endl;
	transfer(N(eosio), N(votedecay), asset::from_string("1000.0000 TLOS"), "Vote decay test");
	auto levy_info = get_vote_counter_bal(N(votedecay), symbol(4, "VOTE").to_symbol_code());
	REQUIRE_MATCHING_OBJECT(levy_info, mvo()
			("owner", "votedecay")
			("decayable_cb", "1000.0000 VOTE")
			("persistent_cb", "0.0000 VOTE")
			("last_decay", now())
		);
	std::cout << "counter balance object exists after initial transfer" << std::endl;
	produce_blocks( 172800 );
	// regvoter and mirrorstake new account
	std::cout << "regvoter for votedecay" << std::endl;
	regvoter(N(votedecay));
	std::cout << "mirrorcast for voter" << std::endl;
	mirrorcast(N(votedecay), symbol(4, "VOTE"));

	//1200 blocks = 5.0000 VOTE or 240 blocks / 1.0000 VOTE
	string levy_amount_after_decay = "280.0000 VOTE";

	levy_info = get_vote_counter_bal(N(votedecay), symbol(4, "VOTE").to_symbol_code());
	std::cout << "now(): " << now() << std::endl;
	std::cout << "counter_balance.last_decay: " << levy_info["last_decay"].as_uint64() << std::endl;
	REQUIRE_MATCHING_OBJECT(levy_info, mvo()
			("owner", "votedecay")
			("decayable_cb", levy_amount_after_decay)
			("persistent_cb", "0.0000 VOTE")
			("last_decay", now() - 86400) //600 is the NUMBER OF BLOCKS PRODUCED in seconds AFTER TRANFER
		);
} FC_LOG_AND_RETHROW()

//TODO: case for reg and unreg ballot
//TODO: shouldn't be able to cycle a ballet that hasn't expired or when the action sender isn't the publisher of the ballot
BOOST_FIXTURE_TEST_CASE( reg_proposal_ballot, eosio_trail_tester ) try {
	account_name publisher = N(voteraaaaaaa);
	uint64_t current_ballot_id = 0;
	uint64_t current_proposal_id = 0;
	string info_url = "Qmasfhuihfaufeanfangnr";
	uint8_t ballot_type = 0;
	uint32_t ballot_length = 1200;
	uint32_t begin_time = now() + 20;
	uint32_t end_time   = now() + ballot_length;
	
	regvoter(publisher);
	regballot(publisher, ballot_type, symbol(4, "VOTE"), begin_time, end_time, info_url);

	auto ballot_info = get_ballot(current_ballot_id);
	REQUIRE_MATCHING_OBJECT(ballot_info, mvo()
		("ballot_id", current_ballot_id)
		("table_id", ballot_type)
		("reference_id", current_proposal_id)
	);

	auto proposal_info = get_proposal(current_proposal_id);
	REQUIRE_MATCHING_OBJECT(proposal_info, mvo()
		("prop_id", current_proposal_id)
		("publisher", publisher.to_string())
		("info_url", info_url)
		("no_count", "0.0000 VOTE")
		("yes_count", "0.0000 VOTE")
		("abstain_count", "0.0000 VOTE")
		("unique_voters", uint32_t(0))
		("begin_time", begin_time)
		("end_time", end_time)
		("cycle_count", 0)
		("status", uint8_t(0))
	);
	produce_blocks( 2 );

	//Malicious unreg: should fail
	account_name non_publisher = N(voteraaaaaab);
	BOOST_REQUIRE_EXCEPTION(unregballot(non_publisher, current_ballot_id),
		eosio_assert_message_exception, eosio_assert_message_is( "cannot delete another account's proposal" ) 
   	);
	
	unregballot(publisher, current_ballot_id);

	ballot_info = get_ballot(current_ballot_id);
	BOOST_REQUIRE_EQUAL(true, ballot_info.is_null());

	proposal_info = get_proposal(current_proposal_id);
	BOOST_REQUIRE_EQUAL(true, proposal_info.is_null());

	begin_time = now() + 20;
	end_time   = now() + ballot_length;

	regballot(publisher, ballot_type, symbol(4, "VOTE"), begin_time, end_time, info_url);

	ballot_info = get_ballot(current_ballot_id);
	REQUIRE_MATCHING_OBJECT(ballot_info, mvo()
		("ballot_id", current_ballot_id)
		("table_id", ballot_type)
		("reference_id", current_proposal_id)
	);

	proposal_info = get_proposal(current_proposal_id);
	REQUIRE_MATCHING_OBJECT(proposal_info, mvo()
		("prop_id", current_proposal_id)
		("publisher", publisher.to_string())
		("info_url", info_url)
		("no_count", "0.0000 VOTE")
		("yes_count", "0.0000 VOTE")
		("abstain_count", "0.0000 VOTE")
		("unique_voters", uint32_t(0))
		("begin_time", begin_time)
		("end_time", end_time)
		("cycle_count", 0)
		("status", uint8_t(0))
	);

	produce_blocks( 2401 );
	
	BOOST_REQUIRE_EXCEPTION(closeballot(non_publisher, current_ballot_id, 1),
		eosio_assert_message_exception, eosio_assert_message_is( "cannot close another account's proposal" ) 
   	);

	closeballot(publisher, current_ballot_id, 1);

	proposal_info = get_proposal(current_proposal_id);
	REQUIRE_MATCHING_OBJECT(proposal_info, mvo()
		("prop_id", current_proposal_id)
		("publisher", publisher.to_string())
		("info_url", info_url)
		("no_count", "0.0000 VOTE")
		("yes_count", "0.0000 VOTE")
		("abstain_count", "0.0000 VOTE")
		("unique_voters", uint32_t(0))
		("begin_time", begin_time)
		("end_time", end_time)
		("cycle_count", 0)
		("status", uint8_t(1))
	);

	//TODO: check unregballot again
	//TODO: this assertion msg may change if we check a proposal to see if it's closed.
	BOOST_REQUIRE_EXCEPTION(unregballot(publisher, current_ballot_id),
		eosio_assert_message_exception, eosio_assert_message_is( "cannot delete proposal once voting has begun" ) 
   	);

} FC_LOG_AND_RETHROW()

//TODO: full flow test
BOOST_FIXTURE_TEST_CASE( full_proposal_flow, eosio_trail_tester ) try {
	//TODO: regballot type 0 and check
	account_name publisher = N(voteraaaaaaa);
	uint64_t current_ballot_id = 0;
	uint64_t current_proposal_id = 0;
	string info_url = "Qmasfhuihfaufeanfangnr";
	uint8_t ballot_type = 0;
	uint32_t ballot_length = 1200 + test_voters.size() * 2;
	uint32_t begin_time = now() + 20;
	uint32_t end_time   = now() + ballot_length;
	regballot(publisher, 0, symbol(4, "VOTE"), begin_time, end_time, info_url);
	
	auto ballot_info = get_ballot(current_ballot_id);
	REQUIRE_MATCHING_OBJECT(ballot_info, mvo()
		("ballot_id", current_ballot_id)
		("table_id", ballot_type)
		("reference_id", current_proposal_id)
	);

	auto proposal_info = get_proposal(current_proposal_id);
	REQUIRE_MATCHING_OBJECT(proposal_info, mvo()
		("prop_id", current_proposal_id)
		("publisher", publisher.to_string())
		("info_url", info_url)
		("no_count", "0.0000 VOTE")
		("yes_count", "0.0000 VOTE")
		("abstain_count", "0.0000 VOTE")
		("unique_voters", uint32_t(0))
		("begin_time", begin_time)
		("end_time", end_time)
		("cycle_count", 0)
		("status", uint8_t(0))
	);
	
	asset no_count = asset(0, symbol(4, "VOTE"));
	asset yes_count = asset(0, symbol(4, "VOTE"));
	asset abstain_count = asset(0, symbol(4, "VOTE"));
	uint32_t unique_voters = 0;
	produce_blocks( 41 );

	fc::variant voter_info = mvo();
	fc::variant vote_receipt_info = mvo();
	asset currency_balance = asset::from_string("0.0000 TLOS");
	asset voter_total = asset::from_string("0.0000 VOTE");
	for (int i = 0; i < test_voters.size(); i++) {
		regvoter(test_voters[i].value);
		voter_info = get_voter(test_voters[i], symbol(4, "VOTE").to_symbol_code());
		REQUIRE_MATCHING_OBJECT(voter_info, mvo()
			("owner", test_voters[i].to_string())
			("tokens", "0.0000 VOTE")
		);
		
		//TODO: mirror stake and check
		mirrorcast(test_voters[i].value, symbol(4, "VOTE"));
		voter_info = get_voter(test_voters[i], symbol(4, "VOTE").to_symbol_code());
		currency_balance = get_currency_balance(N(eosio.token), symbol(4, "TLOS"), test_voters[i].value);
		voter_total = asset::from_string(voter_info["tokens"].as_string());
		BOOST_REQUIRE_EQUAL(currency_balance.get_amount(), voter_total.get_amount());

		uint16_t direction = std::rand() % 3;
		std::cout << "random direction for vote: " << direction << std::endl;
		castvote(test_voters[i].value, current_ballot_id, direction);
		switch(direction) {
			case 0:
				no_count = no_count + voter_total;
				break;
			case 1:
				yes_count = yes_count + voter_total;
				break;
			case 2:
				abstain_count = abstain_count + voter_total;
				break;
		}
		unique_voters++;
		produce_blocks();

		vote_receipt_info = get_vote_receipt(test_voters[i].value, current_ballot_id);
 		std::cout << "direction: " << vote_receipt_info["directions"].get_array()[0] << std::endl;
		REQUIRE_MATCHING_OBJECT(vote_receipt_info, mvo()
			("ballot_id", current_ballot_id)
			("directions", vector<uint16_t> {direction})
			("weight", voter_total.to_string())
			("expiration", end_time)
		);

		deloldvotes(test_voters[i].value, 1);
		vote_receipt_info = get_vote_receipt(test_voters[i].value, current_ballot_id);
		BOOST_REQUIRE_EQUAL(false, vote_receipt_info.is_null());
	}
	std::cout << "local_no_count: " << no_count.to_string() << std::endl;
	std::cout << "local_yes_count: " << yes_count.to_string() << std::endl;
	std::cout << "local_abstain_count: " << abstain_count.to_string() << std::endl;
	proposal_info = get_proposal(current_proposal_id);
	REQUIRE_MATCHING_OBJECT(proposal_info, mvo()
		("prop_id", current_proposal_id)
		("publisher", publisher.to_string())
		("info_url", info_url)
		("no_count", no_count.to_string())
		("yes_count", yes_count.to_string())
		("abstain_count", abstain_count.to_string())
		("unique_voters", unique_voters)
		("begin_time", begin_time)
		("end_time", end_time)
		("cycle_count", 0)
		("status", uint8_t(0))
	);
	
	//produce blocks until expiration period
	do {
		produce_blocks( 100 );
	} while(now() < end_time);
	
	// nextcycle
	begin_time = now() + 20;
	end_time   = now() + ballot_length;
	nextcycle(publisher, current_ballot_id, begin_time, end_time);
	no_count = asset(0, symbol(4, "VOTE"));
	yes_count = asset(0, symbol(4, "VOTE"));
	abstain_count = asset(0, symbol(4, "VOTE"));
	unique_voters = 0;
	produce_blocks( 41 );
	for (int i = 0; i < test_voters.size(); i++) {
		//TODO: castvote and check
		deloldvotes(test_voters[i].value, 1);
		vote_receipt_info = get_vote_receipt(test_voters[i].value, current_ballot_id);
		BOOST_REQUIRE_EQUAL(true, vote_receipt_info.is_null());

		uint16_t direction = std::rand() % 3;
		std::cout << "random direction for vote: " << direction << std::endl;
		castvote(test_voters[i].value, current_ballot_id, direction);
		voter_info = get_voter(test_voters[i], symbol(4, "VOTE").to_symbol_code());
		voter_total = asset::from_string(voter_info["tokens"].as_string());
		switch(direction) {
			case 0:
				no_count = no_count + voter_total;
				break;
			case 1:
				yes_count = yes_count + voter_total;
				break;
			case 2:
				abstain_count = abstain_count + voter_total;
				break;
		}
		produce_blocks();
		vote_receipt_info = get_vote_receipt(test_voters[i].value, current_ballot_id);
		REQUIRE_MATCHING_OBJECT(vote_receipt_info, mvo()
			("ballot_id", current_ballot_id)
			("directions", vector<uint16_t> { direction })
			("weight", voter_total.to_string())
			("expiration", end_time)
		);
		unique_voters++;
	}
	//produce blocks to close nextcycle
	do {
		produce_blocks( 100 );
	} while(now() < end_time);

	//closeballot and check
	closeballot(publisher, current_ballot_id, 1);
	proposal_info = get_proposal(current_proposal_id);
	REQUIRE_MATCHING_OBJECT(proposal_info, mvo()
		("prop_id", current_proposal_id)
		("publisher", publisher.to_string())
		("info_url", info_url)
		("no_count", no_count.to_string())
		("yes_count", yes_count.to_string())
		("abstain_count", abstain_count.to_string())
		("unique_voters", unique_voters)
		("begin_time", begin_time)
		("end_time", end_time)
		("cycle_count", 1)
		("status", 1)
	);
} FC_LOG_AND_RETHROW()

BOOST_FIXTURE_TEST_CASE( full_leaderboard_flow, eosio_trail_tester ) try {
	//TODO: regballot type 0 and check
	account_name publisher = N(voteraaaaaaa);
	uint64_t current_ballot_id = 0;
	uint64_t current_leaderboard_id = 0;
	string info_url = "Qmasfhuihfaufeanfangnr";
	uint8_t ballot_type = 2;
	uint32_t ballot_length = 1200 + test_voters.size() * 2;
	uint32_t begin_time = now() + 20;
	uint32_t end_time   = now() + ballot_length;
	regballot(publisher, ballot_type, symbol(4, "VOTE"), begin_time, end_time, info_url);
	
	auto ballot_info = get_ballot(current_ballot_id);
	REQUIRE_MATCHING_OBJECT(ballot_info, mvo()
		("ballot_id", current_ballot_id)
		("table_id", ballot_type)
		("reference_id", current_leaderboard_id)
	);

	//TODO: set seats
	string candidate1_info = "Qm1";
	std::cout << "addcandidate 1" << std::endl;
	addcandidate(publisher, current_ballot_id, N(voteraaaaaab), candidate1_info);


	string candidate2_info = "Qm2";
	//std::cout << "addcandidate 2" << std::endl;
	//auto trace = addcandidate(publisher, current_ballot_id, N(voteraaaaaac), candidate2_info);
	//std::cout << "console output addcandidate 2: " << trace->action_traces.console << std::endl;

	string candidate3_info = "Qm3";
	//std::cout << "addcandidate 2" << std::endl;
	//addcandidate(publisher, current_ballot_id, N(voteraaaaaad), candidate3_info);

	vector<mvo> candidates;
	candidates.emplace_back(mvo()
		("member", "voteraaaaaab")
		("info_link", candidate1_info)
		("votes", "0.0000 VOTE")
		("status", uint8_t(0))
	);

	candidates.emplace_back(mvo()
		("member", "voteraaaaaac")
		("info_link", candidate2_info)
		("votes", "0.0000 VOTE")
		("status", uint8_t(0))
	);

	candidates.emplace_back(mvo()
		("member", "voteraaaaaad")
		("info_link", candidate3_info)
		("votes", "0.0000 VOTE")
		("status", uint8_t(0))
	);

	auto leaderboard_info = get_leaderboard(publisher, current_leaderboard_id);
	REQUIRE_MATCHING_OBJECT(leaderboard_info, mvo()
		("board_id", current_leaderboard_id)
		("publisher", publisher)
		("info_url", info_url)
		("candidates", candidates)
		("unique_voters", uint64_t(0))
		("voting_symbol", symbol(4, "VOTE").to_string())
		("available_seats", uint8_t(3))
		("begin_time", begin_time)
		("end_time", end_time)
		("status", uint8_t(0))
	);

	uint32_t unique_voters = 0;
	produce_blocks( 41 );

	fc::variant voter_info = mvo();
	fc::variant vote_receipt_info = mvo();
	asset currency_balance = asset::from_string("0.0000 TLOS");
	asset voter_total = asset::from_string("0.0000 VOTE");
	vector<asset> candidate_votes { 
		asset::from_string("0.0000 VOTE"), 
		asset::from_string("0.0000 VOTE"), 
		asset::from_string("0.0000 VOTE")
	};
	for (int i = 0; i < test_voters.size(); i++) {
		regvoter(test_voters[i].value);
		voter_info = get_voter(test_voters[i], symbol(4, "VOTE").to_symbol_code());
		REQUIRE_MATCHING_OBJECT(voter_info, mvo()
			("owner", test_voters[i].to_string())
			("tokens", "0.0000 VOTE")
		);
		
		mirrorcast(test_voters[i].value, symbol(4, "VOTE"));
		voter_info = get_voter(test_voters[i], symbol(4, "VOTE").to_symbol_code());
		currency_balance = get_currency_balance(N(eosio.token), symbol(4, "TLOS"), test_voters[i].value);
		voter_total = asset::from_string(voter_info["tokens"].as_string());
		BOOST_REQUIRE_EQUAL(currency_balance.get_amount(), voter_total.get_amount());

		uint16_t direction = std::rand() % 3;
		std::cout << "random direction for vote: " << direction << std::endl;
		castvote(test_voters[i].value, current_ballot_id, direction);

		candidate_votes[direction] += voter_total;

		unique_voters++;
		produce_blocks();

		vote_receipt_info = get_vote_receipt(test_voters[i].value, current_ballot_id);
 		std::cout << "direction: " << vote_receipt_info["directions"].get_array()[0] << std::endl;
		REQUIRE_MATCHING_OBJECT(vote_receipt_info, mvo()
			("ballot_id", current_ballot_id)
			("directions", vector<uint16_t> {direction})
			("weight", voter_total.to_string())
			("expiration", end_time)
		);

		deloldvotes(test_voters[i].value, 1);
		vote_receipt_info = get_vote_receipt(test_voters[i].value, current_ballot_id);
		BOOST_REQUIRE_EQUAL(false, vote_receipt_info.is_null());
	}

	for (uint8_t i = 0; i < candidates.size(); i++) {
		candidates[i]["votes"] = candidate_votes[i].to_string();
	}

	leaderboard_info = get_leaderboard(publisher, current_leaderboard_id);
	REQUIRE_MATCHING_OBJECT(leaderboard_info, mvo()
		("board_id", current_leaderboard_id)
		("publisher", publisher)
		("info_url", info_url)
		("candidates", candidates)
		("unique_voters", uint64_t(0))
		("voting_symbol", symbol(4, "VOTE").to_string())
		("available_seats", uint8_t(3))
		("begin_time", begin_time)
		("end_time", end_time)
		("status", uint8_t(0))
	);
} FC_LOG_AND_RETHROW()

BOOST_AUTO_TEST_SUITE_END()