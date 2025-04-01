Import("env")


def after_upload(source, target, env):
    print("Delay while uploading...")
    print("Done!")


env.AddPostAction("upload", after_upload)
