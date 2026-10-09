#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MallGenerator.generated.h"

// Предварительное объявление: говорим компилятору, что такой класс существует,
class UInstancedStaticMeshComponent;

// UENUM — пометка для Unreal Header Tool, как UCLASS, только для перечислений.
// enum class — тип с фиксированным списком значений.
// : uint8 — каждое значение занимает 1 байт (числа 0–255)
UENUM()
enum class ECellType : uint8
{
	Outside,   // вне здания
	Empty,     // внутри здания, пока пусто
	ShopZone,  // Полоса у края, будущие магазины и глухие стены
	Corridor,  // коридор
	Shop	   // магазин
};

// Категория — что продаётся, определяет внешний вид
UENUM()
enum class EShopCategory : uint8
{
	Sport,
	Luxury,
	Clothing,
	Grocery,
	Sweets,
	Games,
	Electronics,
	Unfinished,
	Cafe
};

// Состояние — что магазин значит для игрока
UENUM()
enum class EShopState : uint8
{
	Safe,
	StaticMannequins,
	ActiveMannequins,
	Quest,
	Locked
};

// USTRUCT — своя структура Unreal, как FIntPoint, только с нашими полями.
// Префикс F обязателен для структур. GENERATED_BODY нужен так же, как в классе.
// UPROPERTY у полей позволяет редактору показывать их, а движку — сохранять
USTRUCT()
struct FShopData {
	GENERATED_BODY()

	// Левый нижний угол магазина в клетках
	UPROPERTY(VisibleAnywhere)
	FIntPoint Min = FIntPoint::ZeroValue;

	// Ширина и глубина в клетках
	UPROPERTY(VisibleAnywhere)
	FIntPoint Size = FIntPoint::ZeroValue;

	// Клетка магазина, в стене которой стоит дверь
	UPROPERTY(VisibleAnywhere)
	FIntPoint DoorCell = FIntPoint::ZeroValue;

	// Направление от двери в коридор: (1, 0), (-1, 0), (0, 1) или (0, -1)
	UPROPERTY(VisibleAnywhere)
	FIntPoint DoorDir = FIntPoint::ZeroValue;

	UPROPERTY(VisibleAnywhere)
	EShopCategory Category = EShopCategory::Unfinished;

	UPROPERTY(VisibleAnywhere)
	int32 Brand = 0;

	UPROPERTY(VisibleAnywhere)
	EShopState State = EShopState::Safe;
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

	UPROPERTY(EditAnywhere, Category = "Mall")
	bool bGenerateOnBeginPlay = true;

	// VisibleAnywhere вместо EditAnywhere: поле видно в редакторе,
	// но менять его вручную нельзя. Размер мелкой сетки теперь
	// вычисляется в Generate из числа блоков
	UPROPERTY(VisibleAnywhere, Category = "Mall")
	int32 GridWidth = 0;

	UPROPERTY(VisibleAnywhere, Category = "Mall")
	int32 GridHeight = 0;

	UPROPERTY(EditAnywhere, Category = "Mall", meta = (ClampMin = "50.0"))
	float CellSize = 400.f;

	UPROPERTY(EditAnywhere, Category = "Mall", meta = (ClampMin = "1.0"))
	float FloorThickness = 10.f;

	UPROPERTY(EditAnywhere, Category = "Mall|Shape", meta = (ClampMin = "1"))
	int32 ShapeBlocksX = 6;

	UPROPERTY(EditAnywhere, Category = "Mall|Shape", meta = (ClampMin = "1"))
	int32 ShapeBlocksY = 4;

	// Сторона одного блока в клетках мелкой сетки
	UPROPERTY(EditAnywhere, Category = "Mall|Shape", meta = (ClampMin = "1"))
	int32 BlockSize = 12;

	// Какую долю крупной сетки заполнить: 0.6 = 60% блоков.
	// ClampMin и ClampMax не дают ввести значение меньше 0 или больше 1
	UPROPERTY(EditAnywhere, Category = "Mall|Shape", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float ShapeFill = 0.6f;

	// Ширина полосы под магазины вдоль края фигуры, в клетках
	UPROPERTY(EditAnywhere, Category = "Mall|Shape", meta = (ClampMin = "1"))
	int32 ShopStripWidth = 3;

	// Ширина полосы под основной коридор
	UPROPERTY(EditAnywhere, Category = "Mall|Shape", meta = (ClampMin = "1"))
	int32 CorridorWidth = 5;

	// Минимальная длина широкого поперечного коридора, в клетках
	UPROPERTY(EditAnywhere, Category = "Mall|Zones", meta = (ClampMin = "1"))
	int32 MinCrossingLength = 10;

	// Количество коридоров
	UPROPERTY(EditAnywhere, Category = "Mall|Zones", meta = (ClampMin = "0"))
	int32 CrossingCount = 8;

	// Веса ширины коридоров
	UPROPERTY(EditAnywhere, Category = "Mall|Zones")
	TArray<float> CrossingWidthWeights {10.0f, 10.0f, 25.0f, 25.0f, 20.0f};

	// Проверка на длину для широких коридоров (от стольки-то ячеек шириной)
	UPROPERTY(EditAnywhere, Category = "Mall|Zones", meta = (ClampMin = "1"))
	int32 MinLengthFromWidth = 4;

	// Ширина перегородки на основном коридоре
	UPROPERTY(EditAnywhere, Category = "Mall|Zones", meta = (ClampMin = "1"))
	int32 CutThickness = 4;

	// Минимальная длина тупика в клетках: сколько клеток прямого коридора
	UPROPERTY(EditAnywhere, Category = "Mall|Zones", meta = (ClampMin = "1"))
	int32 MinDeadEndLength = 5;

	UPROPERTY(EditAnywhere, Category = "Mall|Zones", meta = (ClampMin = "0"))
	int32 CutCount = 3;

	UPROPERTY(EditAnywhere, Category = "Mall|Zones", meta = (ClampMin = "1"))
	int32 MinSpurLength = 5;

	UPROPERTY(EditAnywhere, Category = "Mall|Zones", meta = (ClampMin = "1"))
	int32 MaxSpurLength = 12;

	UPROPERTY(EditAnywhere, Category = "Mall|Zones", meta = (ClampMin = "0"))
	int32 SpurCount = 4;

	UPROPERTY(VisibleAnywhere, Category = "Mall|Result")
	TArray<FShopData> Shops;

	UFUNCTION(CallInEditor, Category = "Mall")
	void DrawGrid();

	UFUNCTION(CallInEditor, Category = "Mall")
	void Generate();

protected:
	// virtual — функция, которую наследник может заменить своей версией.
	// override — «я заменяю функцию родителя». Если имя или параметры
	virtual void BeginPlay() override;

private:
	TArray<ECellType> Cells;
	TArray<bool> Blocks;
	TArray<int32> CellShopIds;

	// Список точек - центров тупиков.
	TArray<FIntPoint> DeadEnds;

	// Переводит координаты (X, Y) в номер элемента массива.
	int32 GetIndex(int32 X, int32 Y) const;

	// То же, что GetIndex, только для крупной сетки:
	int32 GetBlockIndex(int32 BX, int32 BY) const;

	// FRandomStream — генератор случайных чисел Unreal.
	// Хранит своё зерно и выдаёт из него цепочку чисел.
	FRandomStream Rng;

	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UInstancedStaticMeshComponent> FloorMesh;

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

	// Расставляет плитки пола под клетками коридора
	void BuildFloor();

	// true, если клетка существует в сетке и имеет тип Type.
	// Удобно для проверок: клетка за краем сетки просто даёт false
	bool IsCellType(FIntPoint Cell, ECellType Type) const;

	// Пытается проложить один поперечный коридор. true — если получилось
	bool TryPlaceCrossing();

	// Выбирает рандомную ширину коридора 
	int32 PickCrossingWidth();

	// Функция для проверки что от любой клетки коридора можно дойти от любой другой
	bool AreCorridorsConnected() const;

	// Функция для получения списка всех клеток коридора
	TArray<FIntPoint> GetAllCorridorCells() const;

	// Функция для вставки перегородки
	bool TryPlaceCut();

	// Функция для вставки коридора с тупиком
	bool TryPlaceSpur();


};
