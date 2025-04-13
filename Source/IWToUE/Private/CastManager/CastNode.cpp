#include "CastManager/CastNode.h"

FString FCastNode::GetName() const
{
	return NodeName;
}

void FCastNode::SetName(FString& InName)
{
	NodeName = InName;
}
