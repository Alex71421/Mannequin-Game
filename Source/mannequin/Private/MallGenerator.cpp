#include "MallGenerator.h"
#include "DrawDebugHelpers.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AMallGenerator::AMallGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	// Создаём компонент пола
	// SetupAttachment(родитель) прикрепляет компонент к корню: пол будет
	// двигаться вместе с генератором
	FloorMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Floor"));
	FloorMesh->SetupAttachment(RootComponent);

	// FObjectFinder<Тип>(путь) ищет ассет в проекте по пути.
	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded()) {
		FloorMesh->SetStaticMesh(CubeMesh.Object);
	}
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

			FColor Color;

			switch (Type) {
			case ECellType::Outside:
				continue;
			case ECellType::Empty:
				Color = FColor(80, 80, 80);
				break;
			case ECellType::ShopZone:
				Color = FColor::Yellow;
				break;
			case ECellType::Corridor:
				Color = FColor::Blue;
				break;
			case ECellType::Shop: {
				static const FColor CategoryColors[] = {
					FColor(0, 200, 0),      // Sport       — зелёный
					FColor(255, 215, 0),    // Luxury      — золотой
					FColor(200, 0, 200),    // Clothing    — пурпурный
					FColor(255, 128, 0),    // Grocery     — оранжевый
					FColor(255, 105, 180),  // Sweets      — розовый
					FColor(0, 220, 220),    // Games       — бирюзовый
					FColor(140, 0, 255),    // Electronics — фиолетовый
					FColor(139, 90, 43),    // Unfinished  — коричневый
					FColor(255, 255, 255)   // Cafe        — белый
				};

				const int32 ShopId = CellShopIds[GetIndex(X, Y)];
				Color = CategoryColors[static_cast<int32>(Shops[ShopId].Category)];
				break; } 
			}

			DrawDebugBox(World, Center, HalfSize, Color, true);
		}
	}
}

void AMallGenerator::Generate()
{
	const double StartTime = FPlatformTime::Seconds();
	GridWidth = ShapeBlocksX * BlockSize;
	GridHeight = ShapeBlocksY * BlockSize;

	Cells.Init(ECellType::Outside, GridWidth * GridHeight);
	CellShopIds.Init(INDEX_NONE, GridWidth * GridHeight);
	Shops.Reset();
	DeadEnds.Reset();

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
				// Клетка входит в здание. Пока помечаем её как внутреннюю часть,
				Cells[GetIndex(X, Y)] = ECellType::Empty;
			}
		}
	}

	const int32 MaxDistance = ShopStripWidth + CorridorWidth;

	for (int32 Y = 0; Y < GridHeight; ++Y) {
		for (int32 X = 0; X < GridWidth; ++X) {
			const int32 Index = GetIndex(X, Y);

			// Улицу не трогаем, обрабатываем только клетки здания
			if (Cells[Index] != ECellType::Empty) {
				continue;
			}

			const int32 Distance = GetDistanceToEdge(X, Y, MaxDistance);
			if (Distance <= ShopStripWidth) {
				Cells[Index] = ECellType::ShopZone;
			}
			else if (Distance <= MaxDistance) {
				Cells[Index] = ECellType::Corridor;
			}
		}
	}

	// Генерация коридоров
	int32 CrossingAttempts = 0;
	int32 TotalCrossings = 0;
	while (CrossingAttempts < CrossingCount * 50 && TotalCrossings < CrossingCount) {
		if (TryPlaceCrossing()) {
			++TotalCrossings;
		}
		++CrossingAttempts;
	}
	UE_LOG(LogTemp, Warning, TEXT("Crossings: %d / %d"), TotalCrossings, CrossingCount);

	// Генерация тупиков
	int32 CutAttempts = 0;
	int32 TotalCuts = 0;
	while (CutAttempts < CutCount * 50 && TotalCuts < CutCount) {
		if (TryPlaceCut()) {
			++TotalCuts;
		}
		++CutAttempts;
	}
	UE_LOG(LogTemp, Warning, TEXT("Cuts: %d / %d"), TotalCuts, CutCount);

	// Генерация коридоров с тупиками
	int32 SpurAttempts = 0;
	int32 TotalSpurs = 0;
	while (SpurAttempts < SpurCount * 50 && TotalSpurs < SpurCount) {
		if (TryPlaceSpur()) {
			++TotalSpurs;
		}
		++SpurAttempts;
	}
	UE_LOG(LogTemp, Warning, TEXT("Spurs: %d / %d"), TotalSpurs, SpurCount);

	// генерация магазинов
	TArray<FIntPoint>  FreeCells = GetAllCells(ECellType::ShopZone);
	FreeCells.Append(GetAllCells(ECellType::Empty));

	// Часть 1: заполнение списка кандидатов по "пустая клета + коридор рядом"
	TArray<FShopCandidate> Candidates;
	for (const FIntPoint& Cell : FreeCells) {
		for (int32 Dir = 0; Dir <= 3; ++Dir) {
			FIntPoint Neighbor = GetNeighbor(Cell, Dir);
			if (IsCellType(Neighbor, ECellType::Corridor)) {
				FShopCandidate Candidate;
				Candidate.Cell = Cell;
				Candidate.ToCorridor = Neighbor - Cell;
				Candidates.Add(Candidate);
			}
		}
	}

	// Часть 2: перемешивание списка кандидатов
	for (int32 i = Candidates.Num() - 1; i > 0; --i) {
		const int32 j = Rng.RandRange(0, i);
		Candidates.Swap(i, j);
	}

	// Часть 3: обход и заполнение магазинами
	int32 TotalShops = 0;
	for (const FShopCandidate& Candidate : Candidates) {
		if (TryPlaceShopAt(Candidate.Cell, Candidate.ToCorridor)) {
			++TotalShops;
		}
	}
	UE_LOG(LogTemp, Warning, TEXT("Shops: %d (candidates: %d)"), TotalShops, Candidates.Num());

	// Часть 4. Определение типов и брендов магазинов
	for (FShopData& Shop : Shops) {
		
		if (ShopCategoryWeights.Num() > 0)
		{
			// static_cast возвращает категорию по номеру
			const int32 MaxCategory = static_cast<int32>(EShopCategory::Unfinished);
			const int32 CategoryIndex = FMath::Min(PickWeighted(ShopCategoryWeights), MaxCategory);
			Shop.Category = static_cast<EShopCategory>(CategoryIndex);
		}
		Shop.Brand = Rng.RandRange(0, BrandsPerCategory - 1);
	}

	BuildFloor();
	UE_LOG(LogTemp, Warning, TEXT("Generate: %.1f ms"), (FPlatformTime::Seconds() - StartTime) * 1000.0);
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

bool AMallGenerator::IsCellInBounds(int32 X, int32 Y) const {
	return X >= 0 && Y >= 0 && X < GridWidth && Y < GridHeight;
}

int32 AMallGenerator::GetDistanceToEdge(int32 X, int32 Y, int32 MaxDistance) const {
	// Увеличиваем радиус квадрата: 1, 2, 3 ... MaxDistance
	for (int32 Radius = 1; Radius <= MaxDistance; ++Radius) {
		// DX и DY — сдвиг от клетки (X, Y) внутри квадрата.
		for (int32 DY = -Radius; DY <= Radius; ++DY) {
			for (int32 DX = -Radius; DX <= Radius; ++DX) {
				// Координаты проверяемой клетки внутри квадрата
				const int32 CheckX = X + DX;
				const int32 CheckY = Y + DY;

				// За пределами сетки — это улица, значит край найден
				if (!IsCellInBounds(CheckX, CheckY))
				{
					return Radius;
				}

				if (Cells[GetIndex(CheckX, CheckY)] == ECellType::Outside) {
					return Radius;
				}
			}
		}
	}

	// Ни в одном квадрате улицы не нашлось: клетка глубоко внутри
	return MaxDistance + 1;
}

void AMallGenerator::BuildFloor()
{
	FloorMesh->ClearInstances();

	// Масштаб плитки. Куб имеет размер 100 см, поэтому масштаб CellSize / 100
	// растягивает его на всю клетку: 400 / 100 = 4, то есть 4 метра.
	const FVector Scale(CellSize / 100.0f, CellSize / 100.0f, FloorThickness / 100.f);

	for (int32 Y = 0; Y < GridHeight; ++Y) {
		for (int32 X = 0; X < GridWidth; ++X) {
			if (Cells[GetIndex(X, Y)] != ECellType::Corridor) {
				continue;
			}

			const FVector Location((X + 0.5f) * CellSize, (Y + 0.5f) * CellSize, -FloorThickness * 0.5f);
			FloorMesh->AddInstance(FTransform(FRotator::ZeroRotator, Location, Scale));
		}
	}
}

void AMallGenerator::BeginPlay() {
	Super::BeginPlay();

	if (bGenerateOnBeginPlay) {
		Generate();
	}
}

bool AMallGenerator::IsCellType(FIntPoint Cell, ECellType Type) const {
	if (!IsCellInBounds(Cell.X, Cell.Y)) {
		return false;
	}
	return Cells[GetIndex(Cell.X, Cell.Y)] == Type;
}

bool AMallGenerator::TryPlaceCrossing() {
	// Создание списка AllEmptyCells где все клетки имеют тип Empty
	TArray<FIntPoint> AllEmptyCells = GetAllCells(ECellType::Empty);

	// Проверка на пустой список AllEmptyCells
	if (AllEmptyCells.Num() == 0)
	{
		return false;
	}

	// 1. Случайная клетка. Подходит только внутренняя часть здания
	const FIntPoint Start = AllEmptyCells[Rng.RandRange(0, AllEmptyCells.Num() - 1)];

	// 2. Направление. Step — шаг вдоль будущего коридора,
	// Side — шаг поперёк него, к соседней параллельной полосе
	FIntPoint Step;
	FIntPoint Side;
	if (Rng.RandRange(0, 1) == 0) {
		Step = FIntPoint(1, 0);
		Side = FIntPoint(0, 1);
	}
	else {
		Step = FIntPoint(0, 1);
		Side = FIntPoint(1, 0);
	}

	const int32 Width = PickCrossingWidth();

	// 3. Луч назад: пока следующая клетка назад серая, сдвигаемся на неё.
	// В итоге First — самая дальняя серая клетка в этом направлении
	FIntPoint First = Start;
	while (IsCellType(First - Step, ECellType::Empty)) {
		First = First - Step;
	}

	FIntPoint Last = Start;
	while (IsCellType(Last + Step, ECellType::Empty)) {
		Last = Last + Step;
	}

	const int32 Length = (Last.X - First.X) + (Last.Y - First.Y) + 1;

	// слишком короткий широкий коридор отбрасываем.
	if (Length < MinCrossingLength && Width >= MinLengthFromWidth)
	{
		return false;
	}

	// 4. Проверяем все полосы будущего коридора.
	// Lane - CorridorWidth / 2 — сдвиг полосы от центральной линии.
	// При ширине 5: Lane = 0..4, сдвиги -2, -1, 0, 1, 2 — коридор по центру луча
	for (int32 Lane = 0; Lane < Width; ++Lane) {
		// оффсет от центральной линии. Например (0, -2) от центральной линии
		const FIntPoint Offset = Side * (Lane - Width / 2);

		// За обоими концами полосы должен быть коридор
		if (!IsCellType(First - Step + Offset, ECellType::Corridor) ||
			!IsCellType(Last + Step + Offset, ECellType::Corridor)) {
			return false;
		}

		// Все клетки полосы от First до Last должны быть серыми.
		// First + Step * i — i-я клетка от начала полосы
		for (int32 i = 0; i < Length; ++i) {
			if (!IsCellType(First + Step * i + Offset, ECellType::Empty)) {
				return false;
			}
		}
	}

	// Сдвиги крайних полос от центральной линии. При ширине 5 полосы идут
	// со сдвигами -2..2, значит LowOffset = -2, HighOffset = 2.
	// При ширине 2: сдвиги -1 и 0, LowOffset = -1, HighOffset = 0
	const int32 LowOffset = -(Width / 2);
	const int32 HighOffset = Width - 1 - Width / 2;

	// Сколько серых клеток должно остаться с каждой стороны :
	const int32 MinGap = 2 * InnerShopDepth;

	// 5. Проверка на то что линии сверху и снизу коридора свободны под магазины
	for (int32 i = 0; i < Length; ++i) {
		// i-я клетка центральной линии
		const FIntPoint LineCell = First + Step * i;

		for (int32 Gap = 1; Gap <= MinGap; ++Gap) {
			const FIntPoint Below = LineCell + Side * (LowOffset - Gap);
			const FIntPoint Above = LineCell + Side * (HighOffset + Gap);

			if (IsCellType(Below, ECellType::Corridor) || IsCellType(Above, ECellType::Corridor)) {
				return false;
			}
		}
	}

	// 6. Все проверки пройдены — закрашиваем полосы коридором
	for (int32 Lane = 0; Lane < Width; ++Lane) {
		const FIntPoint Offset = Side * (Lane - Width / 2);
		for (int32 i = 0; i < Length; i++) {
			const FIntPoint Cell = First + Step * i + Offset;
			Cells[GetIndex(Cell.X, Cell.Y)] = ECellType::Corridor;
		}
	}

	return true;
}

int32 AMallGenerator::PickWeighted(const TArray<float>& Weights) {
	if (Weights.Num() == 0) {
		return 0;
	}

	float WeightSum = 0.0f;
	for (int32 i = 0; i < Weights.Num(); ++i) {
		WeightSum += Weights[i];
	}

	// Как только рандомный вес становится <= 0, то значит этот номер возвращаем
	float RandomPickedWeight = Rng.FRandRange(0.f, WeightSum);
	for (int32 i = 0; i < Weights.Num(); ++i) {
		RandomPickedWeight -= Weights[i];
		if (RandomPickedWeight <= 0.0f) {
			return i;
		}
	}
	return Weights.Num() - 1;
}

int32 AMallGenerator::PickCrossingWidth()
{
	// Если веса не заданы, используем ширину основного коридора
	if (CrossingWidthWeights.Num() == 0)
	{
		return CorridorWidth;
	}

	// PickWeighted возвращает номер от 0, а ширина начинается с 1
	return PickWeighted(CrossingWidthWeights) + 1;
}

TArray<FIntPoint> AMallGenerator::GetAllCells(ECellType Type) const {
	// Получаем список всех клеток по типу Type
	TArray<FIntPoint> AllCells;
	for (int32 Y = 0; Y < GridHeight; ++Y) {
		for (int32 X = 0; X < GridWidth; ++X) {
			const FIntPoint CurrentPoint(X, Y);
			if (IsCellType(CurrentPoint, Type)) {
				AllCells.Add(CurrentPoint);
			}
		}
	}
	return AllCells;
}

bool AMallGenerator::AreCorridorsConnected() const {
	
	TArray<FIntPoint> AllCorridorCells = GetAllCells(ECellType::Corridor);

	if (AllCorridorCells.Num() == 0) {
		return true;
	}

	// Стартовая клетка коридора
	const FIntPoint StartCorridorPoint = AllCorridorCells[0];
	
	TArray<FIntPoint> Queue;
	Queue.Add(StartCorridorPoint);

	TArray<bool> Visited;
	Visited.Init(false, GridHeight * GridWidth);
	Visited[GetIndex(StartCorridorPoint.X, StartCorridorPoint.Y)] = true;

	// Заполнение коридоров водой с нарастающим списком
	for (int32 i = 0; i < Queue.Num(); ++i) {
		const FIntPoint CurrentCorridor = Queue[i];

		for (int32 Dir = 0; Dir < 4; ++Dir) {
			const FIntPoint NeighborCell = GetNeighbor(CurrentCorridor, Dir);

			// Проверка на тип клетки и уже залитость
			if (IsCellType(NeighborCell, ECellType::Corridor)) {
				if (!Visited[GetIndex(NeighborCell.X, NeighborCell.Y)]) {
					Queue.Add(NeighborCell);
					Visited[GetIndex(NeighborCell.X, NeighborCell.Y)] = true;
				}
			}
		}
	}

	return Queue.Num() == AllCorridorCells.Num();
}

bool AMallGenerator::TryPlaceCut() {
	TArray<FIntPoint> AllCorridorCells = GetAllCells(ECellType::Corridor);
	if (AllCorridorCells.Num() == 0) {
		return false;
	}

	const FIntPoint StartCell = AllCorridorCells[Rng.RandRange(0, AllCorridorCells.Num() - 1)];

	// Step — шаг вдоль будущего коридора,
	// Side — шаг поперёк него, к соседней параллельной полосе
	FIntPoint Step;
	FIntPoint Side;
	if (Rng.RandRange(0, 1) == 0) {
		Step = FIntPoint(1, 0);
		Side = FIntPoint(0, 1);
	}
	else {
		Step = FIntPoint(0, 1);
		Side = FIntPoint(1, 0);
	}

	FIntPoint First = StartCell;
	while (IsCellType(First - Side, ECellType::Corridor)) {
		First -= Side;
	}

	FIntPoint Last = StartCell;
	while (IsCellType(Last + Side, ECellType::Corridor)) {
		Last += Side;
	}

	const int32 Length = (Last.X - First.X) + (Last.Y - First.Y) + 1;

	if (Length != CorridorWidth) {
		return false;
	}


	// Проверка срезов коридора вдоль Step. Каждый срез — это поперечная линия, сдвинутая на Step * k
	for (int32 Lane = -MinDeadEndLength; Lane < CutThickness + MinDeadEndLength; ++Lane) {
		const FIntPoint Offset = Step * (Lane - CutThickness / 2);

		if (IsCellType(First - Side + Offset, ECellType::Corridor) ||
			IsCellType(Last + Side + Offset, ECellType::Corridor)) {
			return false;
		}

		for (int32 i = 0; i < Length; ++i) {
			if (!IsCellType(First + Side * i + Offset, ECellType::Corridor)) {
				return false;
			}
		}
	}

	// Заполнение клеток коридора пустыми клетками
	for (int32 Lane = 0; Lane < CutThickness; ++Lane) {
		const FIntPoint Offset = Step * (Lane - CutThickness / 2);
		for (int32 i = 0; i < Length; ++i) {
			const FIntPoint CurrentPoint = First + Side * i + Offset;
			Cells[GetIndex(CurrentPoint.X, CurrentPoint.Y)] = ECellType::Empty;
		}
	}

	// Откат если нет соединения
	if (!AreCorridorsConnected()) {
		// Заполнение клеток коридора коридорами
		for (int32 Lane = 0; Lane < CutThickness; ++Lane) {
			const FIntPoint Offset = Step * (Lane - CutThickness / 2);
			for (int32 i = 0; i < Length; ++i) {
				const FIntPoint CurrentPoint = First + Side * i + Offset;
				Cells[GetIndex(CurrentPoint.X, CurrentPoint.Y)] = ECellType::Corridor;
			}
		}
		return false;
	}

	const FIntPoint Center = First + Side * (Length / 2);
	DeadEnds.Add(Center + Step * (-CutThickness / 2 - 1));
	DeadEnds.Add(Center + Step * (CutThickness - CutThickness / 2));

	return true;
}

bool AMallGenerator::TryPlaceSpur() {
	TArray<FIntPoint> AllCorridorCells = GetAllCells(ECellType::Corridor);
	if (AllCorridorCells.Num() == 0) {
		return false;
	}

	const FIntPoint StartCell = AllCorridorCells[Rng.RandRange(0, AllCorridorCells.Num() - 1)];

	const FIntPoint Step = GetNeighbor(StartCell, Rng.RandRange(0, 3)) - StartCell;
	const FIntPoint Side(Step.Y, Step.X);

	const int32 Width = PickCrossingWidth();
	const int32 Length = Rng.RandRange(MinSpurLength, MaxSpurLength);

	// Проверка что начальные клетки - коридор, а остальные по длине + место под магазины - пустые клетки
	for (int32 Lane = 0; Lane < Width; ++Lane) {

		const FIntPoint Offset = Side * (Lane - Width / 2);
		if (!IsCellType(StartCell + Offset, ECellType::Corridor)) {
			return false;
		}

		for (int32 i = 1; i <= Length + InnerShopDepth; ++i) {
			const FIntPoint CurrentPoint = StartCell + Step * i + Offset;
			if (!IsCellType(CurrentPoint, ECellType::Empty)) {
				return false;
			}
		}
	}

	// Не умножал на 2, чтобы добавить вариативности
	const int32 MinGap = InnerShopDepth;

	const int32 LowOffset = -(Width / 2);
	const int32 HighOffset = Width - Width / 2 - 1;

	// Проверка что клетки вокруг коридора пустые
	for (int32 i = 1; i <= Length; ++i) {
		const FIntPoint CenterCell = StartCell + Step * i;

		for (int32 Gap = 1; Gap <= MinGap; ++Gap) {
			const FIntPoint Above = CenterCell + Side * (HighOffset + Gap);
			const FIntPoint Below = CenterCell + Side * (LowOffset - Gap);

			if (!IsCellType(Above, ECellType::Empty) || !IsCellType(Below, ECellType::Empty)) {
				return false;
			}
		}
	}

	// Заполнение коридора
	for (int32 Lane = 0; Lane < Width; ++Lane) {
		const FIntPoint Offset = Side * (Lane - Width / 2);
		for (int32 i = 1; i <= Length; ++i) {
			const FIntPoint CurrentPoint = StartCell + Step * i + Offset;
			Cells[GetIndex(CurrentPoint.X, CurrentPoint.Y)] = ECellType::Corridor;
		}
	}

	DeadEnds.Add(StartCell + Step * Length);
	return true;
}

bool AMallGenerator::TryPlaceShopAt(FIntPoint StartCell, FIntPoint ToCorridor) {
	if (!IsCellType(StartCell, ECellType::ShopZone) && !IsCellType(StartCell, ECellType::Empty)) {
		return false;
	}
	const ECellType StartCellType = Cells[GetIndex(StartCell.X, StartCell.Y)];
	// от коридора, перпендикулярно коридору
	const FIntPoint Inward = ToCorridor * (-1);
	const FIntPoint Along = FIntPoint(ToCorridor.Y, ToCorridor.X);

	const int32 ShopId = Shops.Num();

	int32 Depth;
	if (StartCellType == ECellType::ShopZone) {
		Depth = Rng.RandRange(2, ShopStripWidth);
	}
	else {
		Depth = Rng.RandRange(2, InnerShopDepth);
	}

	int32 Width;
	if (Rng.FRandRange(0, 1) < SquareShopChance) {
		Width = Depth;
	}
	else {
		Width = Rng.RandRange(MinShopWidth, MaxShopWidth);
	}

	// Идеам от большей ширины. Если магазин не вещается - уменьшаем ширину
	for (int32 W = Width; W >= MinShopWidth; --W) {
		bool bFits = true;
		// Проверка, что каждая клетка витрины присоединена к коридору
		for (int32 i = 0; i < W; ++i) {
			if (!IsCellType(StartCell + Along * i + ToCorridor, ECellType::Corridor)) {
				bFits = false;
				break;
			}
		}

		if (!bFits)
		{
			continue;
		}

		// Проверка, что все клетки магазина одинакового типа, что стартовая
		for (int32 Line = 0; Line < Depth && bFits; ++Line) {
			for (int32 i = 0; i < W; ++i) {
				if (!IsCellType(StartCell + Along * i + Inward * Line, StartCellType)) {
					bFits = false;
					break;
				}
			}
		}

		if (!bFits)
		{
			continue;
		}

		// Закраска магазина
		for (int32 Line = 0; Line < Depth; ++Line) {
			for (int32 i = 0; i < W; ++i) {
				FIntPoint CurrentPoint = StartCell + Along * i + Inward * Line;
				Cells[GetIndex(CurrentPoint.X, CurrentPoint.Y)] = ECellType::Shop;
				CellShopIds[GetIndex(CurrentPoint.X, CurrentPoint.Y)] = ShopId;
			}
		}

		FShopData Shop;
		Shop.DoorCell = StartCell + Along * (W / 2);
		Shop.DoorDir = ToCorridor;
		const FIntPoint Corner = StartCell + Along * (W - 1) + Inward * (Depth - 1);
		// ComponentMin берет наименьший Х и Y у обоих 
		Shop.Min = Corner.ComponentMin(StartCell);
		Shop.Size = FIntPoint(FMath::Abs(Corner.X - StartCell.X) + 1, FMath::Abs(Corner.Y - StartCell.Y) + 1);
		Shops.Add(Shop);
		return true;
	}
	
	return false;
}