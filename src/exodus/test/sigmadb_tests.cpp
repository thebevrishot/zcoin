#include "../createpayload.h"
#include "../sigmadb.h"
#include "../tx.h"

#include "../../test/fixtures.h"
#include "../../test/test_bitcoin.h"

#include <leveldb/db.h>

#include <boost/test/unit_test.hpp>

#include <forward_list>
#include <set>
#include <tuple>
#include <vector>

#define TEST_MAX_COINS_PER_GROUP 30

namespace std {

template<class Char, class Traits>
basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os, const exodus::SigmaPublicKey& k)
{
    return os << k.GetCommitment().GetHex();
}

template<class Char, class Traits, class Item1, class Item2>
basic_ostream<Char, Traits>& operator<<(basic_ostream<Char, Traits>& os, const pair<Item1, Item2>& p)
{
    return os << '(' << p.first << ", " << p.second << ')';
}

} // namespace std

namespace exodus {
namespace {

struct MintAdded
{
    PropertyId property;
    DenominationId denomination;
    MintGroupId group;
    MintGroupIndex index;
    SigmaPublicKey pubKey;
    int block;
};

struct MintRemoved
{
    PropertyId property;
    DenominationId denomination;
    SigmaPublicKey pubKey;
};

struct TestSigmaDb : CMPMintList
{
    TestSigmaDb(const boost::filesystem::path& path, bool wipe, uint16_t groupSize = TEST_MAX_COINS_PER_GROUP) :
        CMPMintList(path, wipe, groupSize)
    {
    }

    uint16_t GetGroupSize()
    {
        return CMPMintList::GetGroupSize();
    }

    uint16_t InitGroupSize(uint16_t groupSize)
    {
        return CMPMintList::InitGroupSize(groupSize);
    }

    std::vector<SigmaPublicKey> GetAnonimityGroup(
        PropertyId property,
        DenominationId denomination,
        MintGroupId group,
        size_t count)
    {
        std::vector<SigmaPublicKey> pubs;
        CMPMintList::GetAnonimityGroup(property, denomination, group, count, std::back_inserter(pubs));
        return pubs;
    }

    using CMPMintList::GetAnonimityGroup;
};

class SigmaDbTestingSetup : public TestingSetup
{
public:
    std::vector<MintAdded> mintAdded;
    std::vector<MintRemoved> mintRemoved;

public:
    SigmaDbTestingSetup() : TestingSetup(CBaseChainParams::REGTEST)
    {
    }

    std::unique_ptr<TestSigmaDb> CreateDb(uint16_t groupSize = TEST_MAX_COINS_PER_GROUP)
    {
        return CreateDb("MP_txlist_test", false, groupSize);
    }

    std::unique_ptr<TestSigmaDb> CreateDb(
        const std::string& fileName,
        bool wipe,
        uint16_t groupSize = TEST_MAX_COINS_PER_GROUP)
    {
        std::unique_ptr<TestSigmaDb> db(new TestSigmaDb(pathTemp / fileName, wipe, groupSize));

        sigconns.emplace_front(db->MintAdded.connect([this] (
            PropertyId p,
            DenominationId d,
            MintGroupId g,
            MintGroupIndex i,
            const SigmaPublicKey& k,
            int b) {
            mintAdded.push_back(MintAdded{
                .property = p,
                .denomination = d,
                .group = g,
                .index = i,
                .pubKey = k,
                .block = b
            });
        }));

        sigconns.emplace_front(db->MintRemoved.connect([this] (
            PropertyId p,
            DenominationId d,
            const SigmaPublicKey& k) {
            mintRemoved.push_back(MintRemoved{
                .property = p,
                .denomination = d,
                .pubKey = k
            });
        }));

        return db;
    }

private:
    std::forward_list<boost::signals2::scoped_connection> sigconns;
};

SigmaPublicKey GetNewPubcoin()
{
    SigmaPrivateKey priv;
    priv.Generate();
    return SigmaPublicKey(priv);
}

std::vector<SigmaPublicKey> GetPubcoins(size_t n)
{
    std::vector<SigmaPublicKey> pubs;
    while (n--) {
        pubs.push_back(GetNewPubcoin());
    }
    return pubs;
}

std::vector<SigmaPublicKey> GetFirstN(
    std::vector<SigmaPublicKey>& org, size_t n)
{
    return std::vector<SigmaPublicKey>
        (org.begin(), n > org.size() ? org.end() : org.begin() + n);
}

} // empty namespace

BOOST_FIXTURE_TEST_SUITE(exodus_sigmadb_tests, SigmaDbTestingSetup)

BOOST_AUTO_TEST_CASE(record_one_coin)
{
    auto db = CreateDb();
    auto mint = GetNewPubcoin();
    PropertyId propId = 1;
    DenominationId denom = 0;

    BOOST_CHECK_EQUAL(0, db->GetMintCount(propId, denom, 0));
    BOOST_CHECK_EQUAL(0, db->GetNextSequence());

    BOOST_CHECK_EQUAL(
        std::make_pair(MintGroupId(0), MintGroupIndex(0)),
        db->RecordMint(propId, denom, mint, 100)
    );

    BOOST_CHECK_EQUAL(0, db->GetLastGroupId(propId, denom));
    BOOST_CHECK_EQUAL(1, db->GetMintCount(propId, denom, 0));
    BOOST_CHECK_EQUAL(0, db->GetMintCount(propId, denom, 1));
    BOOST_CHECK_EQUAL(1, db->GetNextSequence());

    BOOST_CHECK_EQUAL(1, mintAdded.size());
    BOOST_CHECK_EQUAL(propId, mintAdded[0].property);
    BOOST_CHECK_EQUAL(denom, mintAdded[0].denomination);
    BOOST_CHECK_EQUAL(0, mintAdded[0].group);
    BOOST_CHECK_EQUAL(0, mintAdded[0].index);
    BOOST_CHECK_EQUAL(mint, mintAdded[0].pubKey);
    BOOST_CHECK_EQUAL(100, mintAdded[0].block);
}

BOOST_AUTO_TEST_CASE(getmint_notfound)
{
    auto db = CreateDb();

    BOOST_CHECK_EXCEPTION(
        db->GetMint(1, 1, 1, 1),
        std::runtime_error,
        [] (const std::runtime_error &e) {
            return std::string("not found sigma mint") == e.what();
        }
    );
}

BOOST_AUTO_TEST_CASE(getmint_test)
{
    auto db = CreateDb();
    auto mint = GetNewPubcoin();
    uint32_t propId = 1;
    uint32_t denom = 0;
    db->RecordMint(propId, denom, mint, 100);

    BOOST_CHECK(
        std::make_pair(mint, int(100)) ==
        db->GetMint(propId, denom, 0, 0)
    );
}

BOOST_AUTO_TEST_CASE(get_anonymityset_no_anycoin)
{
    auto db = CreateDb();

    BOOST_CHECK(db->GetAnonimityGroup(0, 0, 0, 100).empty());
}

BOOST_AUTO_TEST_CASE(get_anonymityset_have_coin_in_other_group)
{
    auto db = CreateDb();
    auto pubs = GetPubcoins(10);
    for (auto const &pub : pubs) {
        db->RecordMint(1, 1, pub, 10);
    }
    BOOST_CHECK(db->GetAnonimityGroup(2, 2, 0, 11).empty());
    BOOST_CHECK(db->GetAnonimityGroup(2, 2, 0, 1).empty());
}

BOOST_AUTO_TEST_CASE(get_anonymityset_have_one_group)
{
    auto db = CreateDb();
    auto pubs = GetPubcoins(10);
    for (auto const &pub : pubs) {
        db->RecordMint(1, 1, pub, 10);
    }

    BOOST_CHECK(pubs == db->GetAnonimityGroup(1, 1, 0, 11));
    BOOST_CHECK(pubs == db->GetAnonimityGroup(1, 1, 0, 10));
    BOOST_CHECK(GetFirstN(pubs, 5) == db->GetAnonimityGroup(1, 1, 0, 5));
}

BOOST_AUTO_TEST_CASE(get_anonymityset_many_properties)
{
    auto db = CreateDb();
    auto pubs = GetPubcoins(10);
    for (auto const &pub : pubs) {
        db->RecordMint(1, 1, pub, 10);
    }

    auto property2Pubs = GetPubcoins(10);
    for (auto const &pub : property2Pubs) {
        db->RecordMint(2, 1, pub, 10);
    }

    BOOST_CHECK(pubs == db->GetAnonimityGroup(1, 1, 0, 11));
    BOOST_CHECK(property2Pubs == db->GetAnonimityGroup(2, 1, 0, 11));
    BOOST_CHECK(pubs == db->GetAnonimityGroup(1, 1, 0, 10));
    BOOST_CHECK(property2Pubs == db->GetAnonimityGroup(2, 1, 0, 10));
    BOOST_CHECK(GetFirstN(pubs, 5) == db->GetAnonimityGroup(1, 1, 0, 5));
    BOOST_CHECK(GetFirstN(property2Pubs, 5) == db->GetAnonimityGroup(2, 1, 0, 5));
}

BOOST_AUTO_TEST_CASE(get_anonymity_set_many_denominations)
{
    auto db = CreateDb();
    auto pubs = GetPubcoins(10);
    auto denom2Pubs = GetPubcoins(10);

    int blocks = 10;
    for (size_t i = 0; i < pubs.size(); i++) {
        db->RecordMint(1, 1, pubs[i], blocks);
        db->RecordMint(1, 2, denom2Pubs[i], 10);
        blocks++;
    }

    BOOST_CHECK(pubs == db->GetAnonimityGroup(1, 1, 0, 11));
    BOOST_CHECK(denom2Pubs == db->GetAnonimityGroup(1, 2, 0, 11));
    BOOST_CHECK(pubs == db->GetAnonimityGroup(1, 1, 0, 10));
    BOOST_CHECK(denom2Pubs == db->GetAnonimityGroup(1, 2, 0, 10));
    BOOST_CHECK(GetFirstN(pubs, 5) == db->GetAnonimityGroup(1, 1, 0, 5));
    BOOST_CHECK(GetFirstN(denom2Pubs, 5) == db->GetAnonimityGroup(1, 2, 0, 5));
}

BOOST_AUTO_TEST_CASE(get_anonymity_set_many_groups)
{
    auto db = CreateDb();
    auto pubs = GetPubcoins(10);
    int countGroup1 = 0;
    for (auto const &pub : pubs) {
        db->RecordMint(1, 1, pub, 10);
        countGroup1++;
    }

    for (; countGroup1 < TEST_MAX_COINS_PER_GROUP; countGroup1++) {
        db->RecordMint(1, 1, pubs[0], 10);
    }

    auto group1Pubs = GetPubcoins(10);
    for (auto const &pub : group1Pubs) {
        db->RecordMint(1, 1, pub, 10);
    }

    BOOST_CHECK(group1Pubs == db->GetAnonimityGroup(1, 1, 1, 11));
    BOOST_CHECK(pubs == db->GetAnonimityGroup(1, 1, 0, 10));
    BOOST_CHECK(group1Pubs == db->GetAnonimityGroup(1, 1, 1, 10));
    BOOST_CHECK(GetFirstN(pubs, 5) == db->GetAnonimityGroup(1, 1, 0, 5));
    BOOST_CHECK(GetFirstN(group1Pubs, 5) == db->GetAnonimityGroup(1, 1, 1, 5));
}

BOOST_AUTO_TEST_CASE(delete_an_empty_set_of_coins)
{
    auto db = CreateDb();

    BOOST_CHECK_EQUAL(0, db->GetNextSequence());
    BOOST_CHECK_NO_THROW(db->DeleteAll(1));
    BOOST_CHECK_EQUAL(0, db->GetNextSequence());
}

BOOST_AUTO_TEST_CASE(delete_block_which_have_no_coins)
{
    auto db = CreateDb();
    auto pubs = GetPubcoins(1);
    db->RecordMint(1, 1, pubs[0], 10); // store at block 10
    BOOST_CHECK_NO_THROW(db->DeleteAll(11)); // delete at block 11
    BOOST_CHECK(pubs == db->GetAnonimityGroup(1, 1, 0, 1));
    BOOST_CHECK_EQUAL(1, db->GetNextSequence());
}

BOOST_AUTO_TEST_CASE(delete_one_coin)
{
    auto db = CreateDb();
    auto pubs = GetPubcoins(1);

    db->RecordMint(1, 1, pubs[0], 10);

    BOOST_CHECK_NO_THROW(db->DeleteAll(10));

    BOOST_CHECK_EQUAL(0, db->GetAnonimityGroup(1, 1, 0, 1).size());
    BOOST_CHECK_EQUAL(0, db->GetNextSequence());

    BOOST_CHECK_EQUAL(1, mintRemoved.size());
    BOOST_CHECK_EQUAL(1, mintRemoved[0].property);
    BOOST_CHECK_EQUAL(1, mintRemoved[0].denomination);
    BOOST_CHECK_EQUAL(pubs[0], mintRemoved[0].pubKey);
}

BOOST_AUTO_TEST_CASE(delete_one_of_two_coin)
{
    auto db = CreateDb();
    auto pubs = GetPubcoins(2);

    db->RecordMint(1, 1, pubs[0], 10); // store at block 10
    db->RecordMint(1, 1, pubs[1], 11); // store at block 11

    BOOST_CHECK_NO_THROW(db->DeleteAll(11)); // delete at block 11

    BOOST_CHECK(GetFirstN(pubs, 1) == db->GetAnonimityGroup(1, 1, 0, 2));
    BOOST_CHECK(GetFirstN(pubs, 1) == db->GetAnonimityGroup(1, 1, 0, 1));
    BOOST_CHECK_EQUAL(1, db->GetNextSequence());

    BOOST_CHECK_EQUAL(1, mintRemoved.size());
    BOOST_CHECK_EQUAL(1, mintRemoved[0].property);
    BOOST_CHECK_EQUAL(1, mintRemoved[0].denomination);
    BOOST_CHECK_EQUAL(pubs[1], mintRemoved[0].pubKey);
}

BOOST_AUTO_TEST_CASE(delete_two_coins_from_two_denominations)
{
    auto db = CreateDb();
    auto pubs = GetPubcoins(2);
    auto denom2Pubs = GetPubcoins(2);

    db->RecordMint(1, 0, pubs[0], 10);
    db->RecordMint(1, 1, denom2Pubs[0], 10);
    db->RecordMint(1, 0, pubs[1], 11);
    db->RecordMint(1, 1, denom2Pubs[1], 12);

    BOOST_CHECK_EQUAL(4, db->GetNextSequence());

    BOOST_CHECK_NO_THROW(db->DeleteAll(11));

    BOOST_CHECK(GetFirstN(pubs, 1) == db->GetAnonimityGroup(1, 0, 0, 2));
    BOOST_CHECK(GetFirstN(denom2Pubs, 1) == db->GetAnonimityGroup(1, 1, 0, 2));
    BOOST_CHECK_EQUAL(2, db->GetNextSequence());

    BOOST_CHECK_EQUAL(2, mintRemoved.size());
    BOOST_CHECK_EQUAL(1, mintRemoved[0].property);
    BOOST_CHECK_EQUAL(1, mintRemoved[0].denomination);
    BOOST_CHECK_EQUAL(denom2Pubs[1], mintRemoved[0].pubKey);

    BOOST_CHECK_EQUAL(1, mintRemoved[1].property);
    BOOST_CHECK_EQUAL(0, mintRemoved[1].denomination);
    BOOST_CHECK_EQUAL(pubs[1], mintRemoved[1].pubKey);
}

BOOST_AUTO_TEST_CASE(delete_two_coins_from_two_properties)
{
    auto db = CreateDb();
    auto pubs = GetPubcoins(2);
    auto property2Pubs = GetPubcoins(2);

    db->RecordMint(1, 0, pubs[0], 10);
    db->RecordMint(2, 0, property2Pubs[0], 10);
    db->RecordMint(1, 0, pubs[1], 11);
    db->RecordMint(2, 0, property2Pubs[1], 12);

    BOOST_CHECK_EQUAL(4, db->GetNextSequence());

    BOOST_CHECK_NO_THROW(db->DeleteAll(11));

    BOOST_CHECK(GetFirstN(pubs, 1) == db->GetAnonimityGroup(1, 0, 0, 2));
    BOOST_CHECK(GetFirstN(property2Pubs, 1) == db->GetAnonimityGroup(2, 0, 0, 2));
    BOOST_CHECK_EQUAL(2, db->GetNextSequence());
}

BOOST_AUTO_TEST_CASE(delete_three_coins_from_two_groups)
{
    auto db = CreateDb();
    auto pubs = GetPubcoins(2);
    auto group1Pubs = GetPubcoins(2);
    db->RecordMint(1, 0, pubs[0], 10);
    db->RecordMint(1, 0, pubs[1], 11);

    size_t coinCount = 2;
    for (;coinCount < TEST_MAX_COINS_PER_GROUP; coinCount++) {
        db->RecordMint(1, 0, pubs[0], 11);
    }

    BOOST_CHECK(std::make_pair(uint32_t(1), uint16_t(0)) ==
        db->RecordMint(1, 0, group1Pubs[0], 12));
    BOOST_CHECK(std::make_pair(uint32_t(1), uint16_t(1)) ==
        db->RecordMint(1, 0, group1Pubs[1], 13));

    BOOST_CHECK_EQUAL(1, db->GetLastGroupId(1, 0));

    BOOST_CHECK_EQUAL(coinCount + 2, db->GetNextSequence());

    BOOST_CHECK_NO_THROW(db->DeleteAll(11));

    BOOST_CHECK(GetFirstN(pubs, 1) == db->GetAnonimityGroup(1, 0, 0, 2));

    BOOST_CHECK(db->GetAnonimityGroup(1, 0, 1, 1).empty());

    BOOST_CHECK_EQUAL(1, db->GetNextSequence());

    // assert last group id of propertyId = 1 and denomination = 0 is decreased to 0
    BOOST_CHECK_EQUAL(0, db->GetLastGroupId(1, 0));
}

BOOST_AUTO_TEST_CASE(get_anonimity_group_by_back_insert_iterator)
{
    auto db = CreateDb();
    size_t cointAmount = 10;
    auto pubs = GetPubcoins(cointAmount);
    for (auto const &pub : pubs) {
        db->RecordMint(1, 1, pub, 10);
    }

    std::vector<SigmaPublicKey> anonimityGroup;
    db->GetAnonimityGroup(1, 1, 0, cointAmount, std::back_inserter(anonimityGroup));

    BOOST_CHECK(pubs == anonimityGroup);
}

BOOST_AUTO_TEST_CASE(get_anonimity_group_by_iterator)
{
    auto db = CreateDb();
    constexpr size_t coinAmount = 10;
    auto pubs = GetPubcoins(coinAmount);
    for (auto const &pub : pubs) {
        db->RecordMint(1, 1, pub, 10);
    }

    std::vector<SigmaPublicKey> anonimityGroup;
    anonimityGroup.resize(coinAmount);
    db->GetAnonimityGroup(1, 1, 0, coinAmount, anonimityGroup.begin());

    BOOST_CHECK(pubs == anonimityGroup);

    std::array<SigmaPublicKey, coinAmount> anonimityGroupArr;
    db->GetAnonimityGroup(1, 1, 0, coinAmount, anonimityGroupArr.begin());
    BOOST_CHECK(pubs == std::vector<SigmaPublicKey>(anonimityGroupArr.begin(), anonimityGroupArr.end()));

    std::list<SigmaPublicKey> anonimityGroupList;
    anonimityGroupList.resize(coinAmount);
    db->GetAnonimityGroup(1, 1, 0, coinAmount, anonimityGroupList.begin());
    BOOST_CHECK(pubs == std::vector<SigmaPublicKey>(anonimityGroupList.begin(), anonimityGroupList.end()));
}

BOOST_AUTO_TEST_CASE(group_size_default)
{
    auto db = CreateDb(0);

    BOOST_CHECK_EQUAL(db->GetGroupSize(), CMPMintList::MAX_GROUP_SIZE);
}

BOOST_AUTO_TEST_CASE(group_size_customsize)
{
    auto db = CreateDb(120);

    BOOST_CHECK_EQUAL(db->GetGroupSize(), 120);
}

BOOST_AUTO_TEST_CASE(group_size_exceed_limit)
{
    BOOST_CHECK_EXCEPTION(
        CreateDb(CMPMintList::MAX_GROUP_SIZE + 1),
        std::invalid_argument,
        [] (const std::invalid_argument& e) {
            return std::string("group size exceed limit") == e.what();
        }
    );
}

BOOST_AUTO_TEST_CASE(use_differnet_group_size_from_database)
{
    CreateDb(10);

    BOOST_CHECK_EXCEPTION(
        CreateDb(11),
        std::invalid_argument,
        [] (const std::invalid_argument& e) {
            return std::string("group size input isn't equal to group size in database")
                == e.what();
        }
    );
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace exodus