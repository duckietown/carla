// Copyright (c) 2025 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"

#include <compiler/disable-ue4-macros.h>
#include <carla/rpc/ObjectLabel.h>
#include <compiler/enable-ue4-macros.h>

#include "Tagger.generated.h"

namespace crp = carla::rpc;

class UCarlaEpisode;
class UTaggedComponent;

/// Sets actors' custom depth stencil value for semantic segmentation according
/// to their meshes.
///
/// Non-static functions present so it can be dropped into the scene for testing
/// purposes.
UCLASS()
class CARLA_API ATagger : public AActor
{
  GENERATED_BODY()

public:

  // Find a tagged component that is attached to the given component.
  template<class T = UTaggedComponent>
  static T* FindTaggedComponent(const USceneComponent* Component);

  /// Set the tag of an actor.
  ///
  /// If bTagForSemanticSegmentation true, activate the custom depth pass. This
  /// pass is necessary for rendering the semantic segmentation. However, it may
  /// add a performance penalty since occlusion doesn't seem to be applied to
  /// objects having this value active.
  static void TagActor(const AActor &Actor, bool bTagForSemanticSegmentation, uint32_t ActorId);


  /// Set the tag of every actor in level.
  ///
  /// If bTagForSemanticSegmentation true, activate the custom depth pass. This
  /// pass is necessary for rendering the semantic segmentation. However, it may
  /// add a performance penalty since occlusion doesn't seem to be applied to
  /// objects having this value active.
  static void TagActorsInLevel(UWorld &World, const UCarlaEpisode &Episode, bool bTagForSemanticSegmentation);
  static void TagActorsInLevel(UWorld &World, bool bTagForSemanticSegmentation);

  static void TagActorsInLevel(ULevel &Level, const UCarlaEpisode &Episode, bool bTagForSemanticSegmentation);

  /// Retrieve the tag of an already tagged component.
  static crp::CityObjectLabel GetTagOfTaggedComponent(const UPrimitiveComponent &Component)
  {
    return static_cast<crp::CityObjectLabel>(Component.CustomDepthStencilValue);
  }

  /// Retrieve the tags of an already tagged actor. CityObjectLabel::None is
  /// not added to the array.
  static void GetTagsOfTaggedActor(const AActor &Actor, TSet<crp::CityObjectLabel> &Tags);

  /// Return true if @a Component has been tagged with the given @a Tag.
  static bool MatchComponent(const UPrimitiveComponent &Component, crp::CityObjectLabel Tag)
  {
    return (Tag == GetTagOfTaggedComponent(Component));
  }

  /// Retrieve the tags of an already tagged actor. CityObjectLabel::None is
  /// not added to the array.
  static FString GetTagAsString(crp::CityObjectLabel Tag);

  /// Method that computes the label corresponding to a folder path
  static crp::CityObjectLabel GetLabelByFolderName(const FString &String);

  /// Method that computes the label corresponding to an specific object
  /// using the folder path in which it is stored.
  ///
  /// The label is taken from the folder directly under a "Static" content
  /// folder, so both /Game/Carla/Static/<Label>/... and
  /// /Game/Duckietown/Static/<Label>/... (at any depth) are supported.
  template <typename T>
  static crp::CityObjectLabel GetLabelByPath(const T *Object) {
    const FString Path = Object->GetPathName();
    TArray<FString> StringArray;
    Path.ParseIntoArray(StringArray, TEXT("/"), false);
    for (int32 i = 0; i + 1 < StringArray.Num(); ++i) {
      if (StringArray[i] == "Static") {
        return GetLabelByFolderName(StringArray[i + 1]);
      }
    }
    return crp::CityObjectLabel::None;
  }

  static void SetStencilValue(UPrimitiveComponent &Component,
    const crp::CityObjectLabel &Label, const bool bSetRenderCustomDepth);

  static FLinearColor GetLabelColor(const uint32_t ActorID, const crp::CityObjectLabel &Label);

  static bool IsThing(const crp::CityObjectLabel &Label);

  ATagger();

protected:

#if WITH_EDITOR
  virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif // WITH_EDITOR

private:

  UPROPERTY(Category = "Tagger", EditAnywhere)
  bool bTriggerTagObjects = false;

  UPROPERTY(Category = "Tagger", EditAnywhere)
  bool bTagForSemanticSegmentation = false;
};
