#include <SmartObject/SmartObjectRegistry.h>

#include <AzCore/Component/TransformBus.h>
#include <AzCore/Name/NameDictionary.h>
#include <AzCore/UnitTest/TestTypes.h>
#include <AzCore/std/smart_ptr/unique_ptr.h>
#include <AzTest/AzTest.h>

namespace GOAT_SmartObject
{
    //! Just enough of a transform for an entity to have a place, without an application to run components.
    class StandInTransform final : public AZ::TransformBus::Handler
    {
    public:
        StandInTransform(AZ::EntityId entity, const AZ::Vector3& position)
            : m_world(AZ::Transform::CreateTranslation(position))
        {
            AZ::TransformBus::Handler::BusConnect(entity);
        }

        ~StandInTransform() override
        {
            AZ::TransformBus::Handler::BusDisconnect();
        }

        void BindTransformChangedEventHandler(AZ::TransformChangedEvent::Handler&) override {}
        void BindParentChangedEventHandler(AZ::ParentChangedEvent::Handler&) override {}
        void BindChildChangedEventHandler(AZ::ChildChangedEvent::Handler&) override {}
        void NotifyChildChangedEvent(AZ::ChildChangeType, AZ::EntityId) override {}
        const AZ::Transform& GetLocalTM() override { return m_world; }
        const AZ::Transform& GetWorldTM() override { return m_world; }
        bool IsStaticTransform() override { return false; }

        AZ::Transform m_world; //!< Where the entity stands.
    };

    class SmartObjectRegistryFixture : public UnitTest::LeakDetectionFixture
    {
    protected:
        void SetUp() override
        {
            UnitTest::LeakDetectionFixture::SetUp();
            AZ::NameDictionary::Create();
            m_registry = AZStd::make_unique<SmartObjectRegistry>();
        }

        void TearDown() override
        {
            m_registry.reset();
            m_transforms.clear();
            m_transforms.shrink_to_fit();
            AZ::NameDictionary::Destroy();
            UnitTest::LeakDetectionFixture::TearDown();
        }

        //! Offers an entity standing at @x along the x axis.
        AZ::EntityId Offer(AZ::u64 id, float x, const char* use, AZ::u32 capacity = 1, const char* owner = "",
            AZStd::vector<AZ::Name> tags = {})
        {
            const AZ::EntityId entity(id);
            m_transforms.push_back(AZStd::make_unique<StandInTransform>(entity, AZ::Vector3(x, 0.0f, 0.0f)));

            SmartObjectDescription description;
            description.m_uses.emplace_back(use);
            description.m_capacity = capacity;
            description.m_owner = owner[0] != '\0' ? AZ::Name(owner) : AZ::Name{};
            description.m_tags = AZStd::move(tags);
            m_registry->Add(entity, AZStd::move(description));
            return entity;
        }

        //! A query for @use from the origin.
        static SmartObjectQuery Ask(const char* use, const char* owner = "", float radius = 50.0f)
        {
            SmartObjectQuery query;
            query.m_use = AZ::Name(use);
            query.m_radius = radius;
            query.m_owner = owner[0] != '\0' ? AZ::Name(owner) : AZ::Name{};
            return query;
        }

        //! An agent id for a test, never null.
        static GOAT::AgentId Agent(AZ::u32 index)
        {
            return GOAT::AgentId(index, 1);
        }

        AZStd::unique_ptr<SmartObjectRegistry> m_registry;
        AZStd::vector<AZStd::unique_ptr<StandInTransform>> m_transforms;
    };

    TEST_F(SmartObjectRegistryFixture, Claim_TakesTheNearestFreeObjectOfferingTheUse)
    {
        Offer(1, 10.0f, "sit");
        const AZ::EntityId near = Offer(2, 3.0f, "sit");
        Offer(3, 1.0f, "sleep");

        const SmartObjectClaim claim = m_registry->Claim(Agent(1), Ask("sit"));
        ASSERT_TRUE(claim.IsValid());
        EXPECT_EQ(claim.m_entity, near);
        EXPECT_TRUE(claim.m_anchor.IsClose(AZ::Vector3(3.0f, 0.0f, 0.0f)));
    }

    TEST_F(SmartObjectRegistryFixture, Claim_RespectsCapacity)
    {
        const AZ::EntityId bench = Offer(1, 2.0f, "sit", 2);

        EXPECT_EQ(m_registry->Claim(Agent(1), Ask("sit")).m_entity, bench);
        EXPECT_EQ(m_registry->Claim(Agent(2), Ask("sit")).m_entity, bench);
        EXPECT_EQ(m_registry->GetFreeSlots(bench), 0u);
        EXPECT_FALSE(m_registry->Claim(Agent(3), Ask("sit")).IsValid()) << "a full object takes nobody else";
    }

    TEST_F(SmartObjectRegistryFixture, Claim_SkipsWhatBelongsToAnotherOwner)
    {
        Offer(1, 1.0f, "rest", 1, "home1");
        const AZ::EntityId mine = Offer(2, 9.0f, "rest", 1, "home2");
        const AZ::EntityId anyones = Offer(3, 20.0f, "rest");

        EXPECT_EQ(m_registry->Claim(Agent(1), Ask("rest", "home2")).m_entity, mine);
        EXPECT_EQ(m_registry->Claim(Agent(2), Ask("rest", "home2")).m_entity, anyones) << "what nobody owns is anyone's";
        EXPECT_TRUE(m_registry->Claim(Agent(3), Ask("rest")).IsValid()) << "asking for no owner accepts any";
    }

    TEST_F(SmartObjectRegistryFixture, Claim_NeedsEveryRequiredTag)
    {
        Offer(1, 1.0f, "eat", 1, "", { AZ::Name("raw") });
        const AZ::EntityId cooked = Offer(2, 5.0f, "eat", 1, "", { AZ::Name("raw"), AZ::Name("cooked") });

        SmartObjectQuery query = Ask("eat");
        query.m_requiredTags = { AZ::Name("cooked") };
        EXPECT_EQ(m_registry->Claim(Agent(1), query).m_entity, cooked);
    }

    TEST_F(SmartObjectRegistryFixture, Claim_GivesBackTheClaimItHeldBefore)
    {
        const AZ::EntityId chair = Offer(1, 1.0f, "sit");
        const AZ::EntityId bed = Offer(2, 1.0f, "sleep");

        ASSERT_EQ(m_registry->Claim(Agent(1), Ask("sit")).m_entity, chair);
        ASSERT_EQ(m_registry->Claim(Agent(1), Ask("sleep")).m_entity, bed);
        EXPECT_EQ(m_registry->GetFreeSlots(chair), 1u);
        EXPECT_EQ(m_registry->FindClaim(Agent(1)).m_entity, bed);
    }

    TEST_F(SmartObjectRegistryFixture, Remove_FreesItsUsers)
    {
        const AZ::EntityId chair = Offer(1, 1.0f, "sit");
        ASSERT_TRUE(m_registry->Claim(Agent(1), Ask("sit")).IsValid());

        m_registry->Remove(chair);
        EXPECT_FALSE(m_registry->FindClaim(Agent(1)).IsValid());
    }

    TEST_F(SmartObjectRegistryFixture, Release_FreesTheSlot)
    {
        const AZ::EntityId chair = Offer(1, 1.0f, "sit");
        ASSERT_TRUE(m_registry->Claim(Agent(1), Ask("sit")).IsValid());

        m_registry->Release(Agent(1));
        EXPECT_EQ(m_registry->GetFreeSlots(chair), 1u);
        EXPECT_FALSE(m_registry->FindClaim(Agent(1)).IsValid());
    }

    TEST_F(SmartObjectRegistryFixture, SetOwner_ChangesWhoMayClaimIt)
    {
        const AZ::EntityId bed = Offer(1, 1.0f, "rest");
        EXPECT_TRUE(m_registry->SetOwner(bed, AZ::Name("home7")));

        EXPECT_FALSE(m_registry->Claim(Agent(1), Ask("rest", "home3")).IsValid());
        EXPECT_EQ(m_registry->Claim(Agent(2), Ask("rest", "home7")).m_entity, bed);
        EXPECT_FALSE(m_registry->SetOwner(AZ::EntityId(99), AZ::Name("home7")));
    }
} // namespace GOAT_SmartObject
