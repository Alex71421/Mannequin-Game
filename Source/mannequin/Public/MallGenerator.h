#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MallGenerator.generated.h"

// UENUM Ч пометка дл€ Unreal Header Tool, как UCLASS, только дл€ перечислений.
// enum class Ч тип с фиксированным списком значений. ќбращатьс€ к ним
// нужно через им€ типа: ECellType::Corridor.
// : uint8 Ч каждое значение занимает 1 байт (числа 0Ц255)
UENUM()
enum class ECellType : uint8
{
	Empty,     // пустота
	Corridor   // коридор
};

UCLASS()
class MANNEQUIN_API AMallGenerator : public AActor
{
	GENERATED_BODY()
	
public:	
	AMallGenerator();

	UPROPERTY(EditAnywhere, Category = "Mall")
	int32 Seed = 1;

	UPROPERTY(EditAnywhere, Category = "Mall")
	bool bRandomSeed = false;

	UPROPERTY(EditAnywhere, Category = "Mall", meta = (ClampMin = "1"))
	int32 GridWidth = 10;

	UPROPERTY(EditAnywhere, Category = "Mall", meta = (ClampMin = "1"))
	int32 GridHeight = 8;

	UPROPERTY(EditAnywhere, Category = "Mall", meta = (ClampMin = "50.0"))
	float CellSize = 400.f;

	UFUNCTION(CallInEditor, Category = "Mall")
	void DrawGrid();

	UFUNCTION(CallInEditor, Category = "Mall")
	void Generate();

private:
	// TArray Ч динамический массив Unreal
	// Ѕез UPROPERTY: показывать в редакторе и сохран€ть его не нужно,
	// мы каждый раз генерируем заново.
	TArray<ECellType> Cells;

	// ѕереводит координаты (X, Y) в номер элемента массива.
	// const в конце Ч функци€ только читает данные и ничего не мен€ет.
	int32 GetIndex(int32 X, int32 Y) const;

	// FRandomStream Ч генератор случайных чисел Unreal.
	// ’ранит своЄ зерно и выдаЄт из него цепочку чисел.
	// Ѕез UPROPERTY: в редакторе его показывать не нужно
	FRandomStream Rng;
};
