#pragma once

struct FCastNodeHeader
{
	uint32 Identifier; // Used to signify which class this node uses
	uint32 NodeSize; // Size of all data and sub data following the node
	uint64 NodeHash; // Unique hash, like an id, used to link nodes together
	uint32 PropertyCount; // The count of properties
	uint32 ChildCount; // The count of direct children nodes

	/*TMap<uint32, FString> TypeSwitcher = {
		{0x746F6F72, "Root"}, // 1953460082
		{0x6C646F6D, "Model"}, // 1818521453
		{0x6873656D, "Mesh"}, // 1752393069
		{0x68736C62, "BlendShape"}, // 1752394850
		{0x6C656B73, "Skeleton"}, // 1818585971
		{0x6D696E61, "Animation"}, // 1835626081
		{0x76727563, "Curve"}, // 1987212643
		{0x564F4D43, "CurveModeOverride"}, // 1448037699
		{0x6669746E, "NotificationTrack"}, // 1718187118
		{0x656E6F62, "Bone"}, // 1701736290
		{0x64686B69, "IKHandle"}, // 1684564841
		{0x74736E63, "Constraint"}, // 1953721955
		{0x6C74616D, "Material"}, // 1819566445
		{0x656C6966, "File"}, // 1701603686
		{0x74736E69, "Instance"}, // 1953721961
		{0x6174656D, "Metadata"} // 1635018093
	};*/
};

enum class ECastPropertyId : uint16_t
{
	Byte = 'b', // <uint8_t>
	Short = 'h', // <uint16_t>
	Integer32 = 'i', // <uint32_t>
	Integer64 = 'l', // <uint64_t>
	Float = 'f', // <float>
	Double = 'd', // <double>
	String = 's', // Null terminated UTF-8 string
	Vector2 = 'v2', // Float precision vector XY
	Vector3 = 'v3', // Float precision vector XYZ
	Vector4 = 'v4' // Float precision vector XYZW
};

struct FCastNodeProperty
{
	uint16 Identifier; // The element type of this property
	ECastPropertyId DataType;
	uint16 NameSize; // The size of the name of this property
	uint32 ArrayLength; // The number of elements this property contains (1 for single)

	FString PropertyName;

	TArray<uint8> ByteArray;
	TArray<uint16> ShortArray;
	TArray<uint32> IntArray;
	TArray<uint64> Int64Array;
	TArray<float> FloatArray;
	TArray<double> DoubleArray;
	FString StringData;
	TArray<FVector2f> Vector2Array;
	TArray<FVector3f> Vector3Array;
	TArray<FVector4f> Vector4Array;
};

class FCastNode
{
public:
	FCastNode()
	{
	}

	FString GetName() const;
	void SetName(FString& InName);

	FCastNodeHeader Header;
	TArray<FCastNodeProperty> Properties;
	TArray<int32> ChildNodes;

private:
	FString NodeName;
};
