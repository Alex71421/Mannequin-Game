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

	// FIntPoint — структура Unreal из двух целых чисел X и Y,
	TArray<FIntPoint> Filled;
	Filled.Add(FIntPoint(StartX, StartY));
	int32 Attempts = 0;

	while (Filled.Num() < TargetBlocks && Attempts < 1000) {
		++Attempts;

		// Берём из массива случайный блок фигуры
		const FIntPoint From = Filled[Rng.RandRange(0, Filled.Num() - 1)];
		const int32 Dir = Rng.RandRange(0, 3);

		FIntPoint Next = From;
		switch (Dir) {
		case 0 :
			Next.Y += 1;
			break;
		case 1:
			Next.X += 1;
			break;
		case 2:
			Next.Y -= 1;
			break;
		case 3:
			Next.X -= 1;
			break;
		}

		// Проверяем, что сосед не вышел за пределы крупной сетки.
		if (Next.X < 0 || Next.Y < 0 || Next.X >= ShapeBlocksX || Next.Y >= ShapeBlocksY) {
			continue;
		}

		// Если сосед уже входит в фигуру, пристраивать нечего
		if (Blocks[GetBlockIndex(Next.X, Next.Y)])
		{
			continue;
		}

		Blocks[GetBlockIndex(Next.X, Next.Y)] = true;
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