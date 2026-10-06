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
	Cells.Init(ECellType::Empty, GridWidth * GridHeight);

	if (bRandomSeed)
	{
		Seed = FMath::Rand();
	}

	// Initialize(зерно): запускает цепочку случайных чисел с начала
	// от зерна Seed. После этого при одном и том же Seed
	// все следующие вызовы Rng выдают одни и те же числа
	Rng.Initialize(Seed);

	for (int32 Y = 0; Y < GridHeight; ++Y)
	{
		for (int32 X = 0; X < GridWidth; ++X)
		{
			if (Rng.FRand() < 0.3f)
			{
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