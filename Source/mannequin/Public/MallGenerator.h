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
	Outside,   // вне здания
	Empty,     // внутри здания, пока пусто
	ShopZone,  // Полоса у края, будущие магазины и глухие стены
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
	int32 BlockSize = 12;

	// Какую долю крупной сетки заполнить: 0.6 = 60% блоков.
	// ClampMin и ClampMax не дают ввести значение меньше 0 или больше 1
	UPROPERTY(EditAnywhere, Category = "Mall|Shape", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShapeFill = 0.6f;

	// Ширина полосы под магазины вдоль края фигуры, в клетках
	UPROPERTY(EditAnywhere, Category = "Mall|Shape", meta = (ClampMin = "1"))
	int32 ShopStripWidth = 2;

	// Ширина полосы под магазины вдоль края фигуры, в клетках
	UPROPERTY(EditAnywhere, Category = "Mall|Shape", meta = (ClampMin = "1"))
	int32 CorridorWidth  = 2;

	UFUNCTION(CallInEditor, Category = "Mall")
	void DrawGrid();

	UFUNCTION(CallInEditor, Category = "Mall")
	void Generate();

private:
	TArray<ECellType> Cells;
	TArray<bool> Blocks;

	// Переводит координаты (X, Y) в номер элемента массива.
	int32 GetIndex(int32 X, int32 Y) const;

	// То же, что GetIndex, только для крупной сетки:
	int32 GetBlockIndex(int32 BX, int32 BY) const;

	// FRandomStream — генератор случайных чисел Unreal.
	// Хранит своё зерно и выдаёт из него цепочку чисел.
	FRandomStream Rng;

	// Возвращает координаты соседнего блока в направлении Dir:
	// 0 — вверх, 1 — вправо, 2 — вниз, 3 — влево
	FIntPoint GetNeighbor(FIntPoint Point, int32 Dir) const;

	// true, если блок с такими координатами есть в крупной сетке
	bool IsBlockInBounds(FIntPoint Point) const;

	// true, если в фигуре есть замкнутая пустота
	bool HasHoles() const;

	// true, если клетка с такими координатами есть в мелкой сетке
	bool IsCellInBounds(int32 X, int32 Y) const;

	// Расстояние от клетки до ближайшей улицы, от 1 до MaxDistance.
	// Если улицы в пределах MaxDistance нет, возвращает MaxDistance + 1
	int32 GetDistanceToEdge(int32 X, int32 Y, int32 MaxDistance) const;
};
