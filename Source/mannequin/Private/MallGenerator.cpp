#include "MallGenerator.h"
#include "DrawDebugHelpers.h"

AMallGenerator::AMallGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AMallGenerator::DrawGrid()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Num() — количество элементов массива. Если сетку ещё не сгенерировали
	// или размер поменялся, обращение к Cells вышло бы за границы массива
	// и уронило редактор. Поэтому выходим.
	if (Cells.Num() != GridWidth * GridHeight)
	{
		return;
	}

	FlushPersistentDebugLines(World);

	const FVector Origin = GetActorLocation();
	const FVector HalfSize(CellSize * 0.5f, CellSize * 0.5f, 10.f);

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			const FVector Center = Origin + FVector((X + 0.5f) * CellSize, (Y + 0.5f) * CellSize, 0.f);

			// Берём тип клетки из массива
			const ECellType Type = Cells[GetIndex(X, Y)];

			// Тернарный оператор: условие ? если_да : если_нет.
			FColor Color = FColor(80, 80, 80);

			// Если клетка — коридор, меняем цвет на синий
			if (Type == ECellType::Corridor)
			{
				Color = FColor::Blue;
			}

			DrawDebugBox(World, Center, HalfSize, Color, true);
		}
	}
}

void AMallGenerator::Generate()
{
	GridWidth = ShapeBlocksX * BlockSize;
	GridHeight = ShapeBlocksY * BlockSize;

	Cells.Init(ECellType::Empty, GridWidth * GridHeight);

	if (bRandomSeed) {
		Seed = FMath::Rand();
	}
	Rng.Initialize(Seed);

	Blocks.Init(false, ShapeBlocksX * ShapeBlocksY);

	// Координаты центрального блока.
	const int32 StartX = ShapeBlocksX / 2;
	const int32 StartY = ShapeBlocksY / 2;
	Blocks[GetBlockIndex(StartX, StartY)] = true;

	// Сколько блоков должно быть в фигуре.
	// FMath::RoundToInt(число) округляет дробное число до ближайшего целого
	// FMath::Max(a, b) возвращает большее из двух
	const int32 TargetBlocks = FMath::Max(1, FMath::RoundToInt(ShapeBlocksX * ShapeBlocksY * ShapeFill));


	TArray<FIntPoint> Filled;
	Filled.Add(FIntPoint(StartX, StartY));
	int32 Attempts = 0;

	while (Filled.Num() < TargetBlocks && Attempts < 1000) {
		++Attempts;

		// Берём из массива случайный блок фигуры и случайного соседа
		const FIntPoint From = Filled[Rng.RandRange(0, Filled.Num() - 1)];
		FIntPoint Next = GetNeighbor(From, Rng.RandRange(0, 3));
		
		if (!IsBlockInBounds(Next)) {
			continue;
		}

		const int32 NextIndex = GetBlockIndex(Next.X, Next.Y);

		if (Blocks[NextIndex]) {
			continue;
		}

		Blocks[NextIndex] = true;

		if (HasHoles()) {
			Blocks[NextIndex] = false;
			continue;
		}

		Filled.Add(Next);
	}

	for (int32 Y = 0; Y < GridHeight; ++Y) {
		for (int32 X = 0; X < GridWidth; ++X) {
			const int32 BX = X / BlockSize;
			const int32 BY = Y / BlockSize;

			if (Blocks[GetBlockIndex(BX, BY)]) {
				Cells[GetIndex(X, Y)] = ECellType::Corridor;
			}
		}
	}

	DrawGrid();
}

int32 AMallGenerator::GetIndex(int32 X, int32 Y) const
{
	return Y * GridWidth + X;
}

int32 AMallGenerator::GetBlockIndex(int32 BX, int32 BY) const {
	return BY * ShapeBlocksX + BX;
}

FIntPoint AMallGenerator::GetNeighbor(FIntPoint Point, int32 Dir) const {
	FIntPoint Result = Point;
	switch (Dir)
	{
	case 0:
		Result.Y += 1;
		break;
	case 1:
		Result.X += 1;
		break;
	case 2:
		Result.Y -= 1;
		break;
	default:
		Result.X -= 1;
		break;
	}
	return Result;
}

bool AMallGenerator::IsBlockInBounds(FIntPoint Point) const {
	return Point.X >= 0 && Point.Y >= 0 && Point.X < ShapeBlocksX && Point.Y < ShapeBlocksY;
}

bool  AMallGenerator::HasHoles() const {
	// Для каждого блока: дошла ли до него вода. Сначала везде false — «сухо»
	TArray<bool> Visited;
	Visited.Init(false, ShapeBlocksX * ShapeBlocksY);

	// Очередь блоков, из которых вода ещё будет растекаться
	TArray<FIntPoint> Queue;

	// 1. Наливаем воду во все пустые блоки по краю сетки
	for (int32 BY = 0; BY < ShapeBlocksY; ++BY) {
		for (int32 BX = 0; BX < ShapeBlocksX; ++BX) {
			// Блок на краю, если он в первом или последнем столбце или ряду
			const bool bOnEdge = BX == 0 || BY == 0 || BX == ShapeBlocksX - 1 || BY == ShapeBlocksY - 1;
			const int32 Index = GetBlockIndex(BX, BY);

			if (bOnEdge && !Blocks[Index]) {
				Visited[Index] = true;
				Queue.Add(FIntPoint(BX, BY));
			}
		}
	}

	// 2. Растекаемся. Берём блоки из очереди по порядку и заливаем их пустых соседей.
	for (int32 i = 0; i < Queue.Num(); ++i) {
		const FIntPoint Current = Queue[i];

		// Проверяем всех четырёх соседей
		for (int32 Dir = 0; Dir < 4; ++Dir) {
			const FIntPoint Next = GetNeighbor(Current, Dir);

			if (!IsBlockInBounds(Next)) {
				continue;
			}

			const int32 NextIndex = GetBlockIndex(Next.X, Next.Y);
			// Сквозь здание вода не течёт, а уже залитый блок второй раз не заливаем
			if (Blocks[NextIndex] || Visited[NextIndex]) {
				continue;
			}

			Visited[NextIndex] = true;
			Queue.Add(Next);
		}
	}

	// 3. Ищем пустой блок, до которого вода не дошла.
	for (int32 i = 0; i < Blocks.Num(); ++i) {
		// Если блок не посещался и блок не здание
		if (!Visited[i] && !Blocks[i]) {
			return true;
		}
	}

	return false;
}