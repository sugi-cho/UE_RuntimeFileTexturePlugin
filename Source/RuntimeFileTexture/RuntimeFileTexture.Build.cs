using UnrealBuildTool;

public class RuntimeFileTexture : ModuleRules
{
	public RuntimeFileTexture(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"MediaAssets"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"ImageWrapper",
			"Projects"
		});

		if (Target.Platform == UnrealTargetPlatform.Win64)
		{
			PublicDefinitions.Add("RUNTIME_FILE_TEXTURE_WINDOWS=1");
		}
		else
		{
			PublicDefinitions.Add("RUNTIME_FILE_TEXTURE_WINDOWS=0");
		}
	}
}
