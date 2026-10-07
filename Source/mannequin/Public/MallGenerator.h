#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MallGenerator.generated.h"

// UENUM — пометка для Unreal Header Tool, как UCLASS, только для перечислений.
// enum class — тип с фиксированным списком значений. Обращаться к ним
// нужно через имя типа: ECellType::Corridor.
// : uint8 — каждое значение занимает 1 байт (числа 0–255)
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

	// VisibleAnywhere вместо EditAnywhere: поле видно в редакторе,
	// но менять его вручную нельзя. Размер мелкой сетки теперь
	// вычисляется в Generate из числа блоков
	UPROPERTY(VisibleAnywhere, Category = "Mall")
	int32 GridWidth = 0;

	UPROPERTY(VisibleAnywhere, Category = "Mall")
	int32 GridHeight = 0;

	UPROPERTY(EditAnywhere, Category = "Mall", meta = (ClampMin = "50.0"))
	float CellSize = 400.f;

	UPROPERTY(EditAnywhere, Category = "Mall|Shape", meta = (ClampMin = "1"))
	int32 ShapeBlocksX = 6;

	UPROPERTY(EditAnywhere, Category = "Mall|Shape", meta = (ClampMin = "1"))
	int32 ShapeBlocksY = 4;

	// Сторона одного блока в клетках мелкой сетки (8 клеток × 4 м = 32 м)
	UPROPERTY(EditAnywhere, Category = "Mall|Shape", meta = (ClampMin = "1"))
	int32 BlockSize = 8;

	// Какую долю крупной сетки заполнить: 0.6 = 60% блоков.
	// ClampMin и ClampMax не дают ввести значение меньше 0 или больше 1
	UPROPERTY(EditAnywhere, Category = "Mall|Shape", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShapeFill = 0.6f;

	UFUNCTION(CallInEditor, Category = "Mall")
	void DrawGrid();

	UFUNCTION(CallInEditor, Category = "Mall")
	void Generate();

private:
	TArray<ECellType> Cells;

	// Крупная сетка: для каждого блока true — входит в фигуру, false — нет.
	// Хранится так же одной строкой, как Cells
	TArray<bool> Blocks;

	// Переводит координаты (X, Y) в номер элемента массива.
	// const в конце — функция только читает данные и ничего не меняет.
	int32 GetIndex(int32 X, int32 Y) const;

	// То же, что GetIndex, только для крупной сетки:
	// переводит координаты блока (BX, BY) в номер в массиве Blocks
	int32 GetBlockIndex(int32 BX, int32 BY) const;

	// FRandomStream — генератор случайных чисел Unreal.
	// Хранит своё зерно и выдаёт из него цепочку чисел.
	// Без UPROPERTY: в редакторе его показывать не нужно
	FRandomStream Rng;
};
